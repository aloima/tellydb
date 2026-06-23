#include <telly.h>
#include "file.h"

static int fd = -1;
static bool saving = false;
static uint16_t block_capacity;

off_t read_file(const int fd, const off_t file_size, char *block, const uint16_t block_size, const uint16_t filled_block_size);

int open_database_fd(uint32_t *server_age) {
  if ((fd = open_file(server->conf->data_file, 0)) == -1)
    return -1;

  struct stat sostat;
  ASSERT(stat(server->conf->data_file, &sostat), ==, 0);

  const off_t file_size = sostat.st_size;
  block_capacity = sostat.st_blksize;

  if (file_size == 0) {
    const string_t database_name = CREATE_STRING(server->conf->database_name, strlen(server->conf->database_name));
    Database *database = create_database(database_name, DATABASE_INITIAL_SIZE);

    if (database == NULL) {
      write_log(LOG_ERR, "Cannot create empty database, out of memory.");
      goto GRACEFUL_SHUTDOWN;
    }

    set_main_database(database);
    write_log(LOG_INFO, "Database file is empty, loaded password and data count: 0");
    *server_age = 0;

    return 0;
  }

  char *block = NULL;

  if (posix_memalign((void **) &block, block_capacity, block_capacity) != 0) {
    write_log(LOG_ERR, "Cannot read database file, out of memory.");
    goto GRACEFUL_SHUTDOWN;
  }

  const clock_t start = clock();
  ASSERT(start, !=, INVALID_CLOCK);

  if (read(fd, block, block_capacity) == -1) {
    write_log(LOG_ERR, "Cannot read headers because of OS-specific problem");
    goto GRACEFUL_SHUTDOWN;
  }

  if (block[0] != 0x18 || block[1] != 0x10) {
    write_log(LOG_ERR, "Invalid headers for database file, file is closed.");
    goto GRACEFUL_SHUTDOWN;
  }

  ASSERT(memcpy(server_age, block + 2, 8), !=, NULL);

  const uint16_t filled_block_size = get_authorization_from_file(fd, block, block_capacity);
  const off_t data_count = read_file(fd, file_size, block, block_capacity, filled_block_size);
  if (data_count == -1) {
    write_log(LOG_ERR, "Cannot read database file, out of memory.");
    return -1;
  }

  const clock_t current = clock();
  ASSERT(current, !=, INVALID_CLOCK);

  write_log(LOG_INFO,
    "Read database file in %.3f seconds. Loaded password count: %u, loaded data count: %d",
    ((float) (current - start)) / CLOCKS_PER_SEC, get_password_count(), data_count
  );

  free(block);
  return 0;

  GRACEFUL_SHUTDOWN: {
    if (fd != -1)
      ASSERT(close(fd), ==, 0);

    if (block != NULL)
      free(block);

    return -1;
  }
}

int close_database_fd() {
  while (saving) {
    usleep(100);
  }

  if (lockf(fd, F_ULOCK, 0) == -1) {
    write_log(LOG_ERR, "The database file cannot be unlocked because of OS-specific problem.");
  }

  if (close(fd) == 0)
    return 0;
  else
    return -1;
}

static inline void log_save_io_error(const char *what) {
  switch (errno) {
    case EIO:
      write_log(LOG_ERR, "%s: I/O error.", what);
      break;
    case ENOSPC:
      write_log(LOG_ERR, "%s: no space left on device.", what);
      break;
    case EDQUOT:
      write_log(LOG_ERR, "%s: disk quota exceeded.", what);
      break;
    case EFBIG:
      write_log(LOG_ERR, "%s: file size limit exceeded.", what);
      break;
    case EBADF:
      write_log(LOG_ERR, "%s: bad file descriptor.", what);
      break;
    case EINTR:
      write_log(LOG_ERR, "%s: interrupted by signal.", what);
      break;
    default:
      write_log(LOG_ERR, "%s: errno %d.", what, errno);
      break;
  }
}

typedef struct State {
  off_t size; // Represents calculated file size
  off_t block_size;
  const off_t block_capacity;

  char *data; // Represents buffer that KeyValue will be written into individually
  char *block; // Represents buffer will be written into database file
} State;

static inline void interrupt_dumping_into_file(State *state) {
  if (state->data) free(state->data);
  if (state->block) free(state->block);
  saving = false;
  close_database_fd();
}

static inline void dump_into_file(HashTableElement element, void *external) {
  const HashTableKeyValue *kv_element = (HashTableKeyValue *) ((void *) &element);
  KeyValue *kv = kv_element->value;
  State *state = (State *) external;

  char *block = state->block;
  char *data = state->data;
  const off_t block_capacity = state->block_capacity;

  const off_t kv_size = generate_value(&data, kv);
  const uint32_t block_count = ((state->block_size + kv_size + block_capacity - 1) / block_capacity);

  if (block_count != 1) {
    off_t remaining = kv_size;
    const uint16_t complete = (block_capacity - state->block_size);

    memcpy(block + state->block_size, data, complete);

    if (write(fd, block, block_capacity) == -1) {
      interrupt_dumping_into_file(state);
      return;
    }

    remaining -= complete;

    if (remaining > block_capacity) {
      do {
        memcpy(block, data + (kv_size - remaining), block_capacity);

        if (write(fd, block, block_capacity) == -1) {
          interrupt_dumping_into_file(state);
          return;
        }

        remaining -= block_capacity;
      } while (remaining > block_capacity);
    }

    state->block_size = remaining;
    memcpy(block, data + (kv_size - remaining), state->block_size);
  } else {
    memcpy(block + state->block_size, data, kv_size);
    state->block_size += kv_size;
  }

  state->size += kv_size;
}

int save_data(const uint32_t server_age) {
  if (saving)
    return -1;

  saving = true;

  char *block = NULL;
  char *data = NULL;
  bool io_failed = false;
  int ret = -1;

  ASSERT(lseek(fd, 0, SEEK_SET), !=, -1);

  if (posix_memalign((void **) &block, block_capacity, block_capacity) != 0) {
    write_log(LOG_ERR, "Cannot allocate aligned memory for database block, out of memory.");
    block = NULL; // posix_memalign leaves *memptr undefined on failure
    goto cleanup;
  }

  memset(block, 0, block_capacity);

  off_t block_size; // `block_size` represents filled block size
  off_t size = 0;   // `size` represents total calculated file size
  generate_headers(block, server_age);

  {
    struct Password **passwords = get_passwords();
    const uint32_t password_count = get_password_count();
    const uint8_t password_count_byte_count = (password_count != 0) ? (log2(password_count) + 1) : 0;
    block_size = (sizeof(DATABASE_FILE_CONSTANT) + sizeof(server->age) + 1) + password_count_byte_count;

    block[sizeof(DATABASE_FILE_CONSTANT) + sizeof(server->age)] = password_count_byte_count;
    memcpy(block + (sizeof(DATABASE_FILE_CONSTANT) + sizeof(server->age) + 1), &password_count, password_count_byte_count);

    if (password_count == 0) {
      size += block_size;
    }

    for (uint32_t i = 0; i < password_count; ++i) {
      struct Password *password = passwords[i];
      const uint32_t new_length = (block_size + 49);

      if (new_length > block_capacity) {
        const uint32_t allowed = (new_length - block_capacity);
        memcpy(block + block_size, password->data, allowed);

        if (write(fd, block, block_capacity) == -1) {
          log_save_io_error("Cannot write passwords block to database file");
          io_failed = true;
          goto cleanup;
        }

        const uint32_t remaining = (48 - allowed); // remaining byte count except permissions
        memcpy(block, password->data, remaining);
        block[remaining] = password->permissions;
        block_size = (remaining + 1);
        size += block_capacity + block_size;
      } else {
        memcpy(block + block_size, password->data, 48);
        block[block_size + 48] = password->permissions;
        block_size = new_length;
        size += block_size;
      }
    }
  }

  {
    LinkedListNode *node = get_databases()->begin;

    while (node) {
      Database *database = (Database *) node->data;
      const uint64_t count = database->data->size.count;

      memcpy(block + block_size, &count, 8);
      block_size += 8;
      size += generate_string_value(&block, &block_size, &database->name) + 8;

      uint64_t data_size = 0;
      foreach_hashtable(database->data, get_maximum_keyvalue_size, &data_size);

      if (data_size == 0) {
        node = node->next;
        continue;
      }

      data = malloc(data_size);
      if (!data) {
        write_log(LOG_ERR, "Cannot allocate buffer for KV data while saving, out of memory.");
        goto cleanup;
      }

      State state = { size, block_size, block_capacity, data, block };
      foreach_hashtable(database->data, dump_into_file, &state);

      // required to exchange, `size` and `block_size` will be used after
      size = state.size;
      block_size = state.block_size;

      free(data);
      data = NULL;
      node = node->next;
    }
  }

  if (block_size != block_capacity) {
    if (write(fd, block, block_capacity) == -1) {
      log_save_io_error("Cannot write final block to database file");
      io_failed = true;
      goto cleanup;
    }
  }

  if (ftruncate(fd, size) == -1) {
    log_save_io_error("Cannot truncate database file to actual data size");
    io_failed = true;
    goto cleanup;
  }

  ret = 0;

cleanup:
  if (data) free(data);
  if (block) free(block);
  saving = false;
  if (io_failed) close_database_fd();
  return ret;
}

void *save_thread(void *arg) {
  const uint64_t *server_age = arg;
  save_data(*server_age);

  pthread_exit(NULL);
}

BackgroundSavingStatus bg_save(const uint32_t server_age) {
  if (saving)
    return BGSAVE_ALREADY_SAVING;

  pthread_t thread;
  const int code = pthread_create(&thread, NULL, save_thread, (uint32_t *) &server_age);
  if (code == EAGAIN)
    return BGSAVE_THREAD_FAILED;

  ASSERT(pthread_detach(thread), ==, 0);

  return BGSAVE_SUCCESSFUL;
}
