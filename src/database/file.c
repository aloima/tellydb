#include <telly.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

#include <gmp.h>

static int fd = -1;
static bool saving = false;
static uint16_t block_size;

const bool open_database_fd(struct Configuration *conf, uint32_t *server_age) {
  if ((fd = open_file(conf->data_file, 0)) == -1) {
    return false;
  }

  struct stat sostat;
  stat(conf->data_file, &sostat);

  const off_t file_size = sostat.st_size;;
  block_size = sostat.st_blksize;

  if (file_size != 0) {
    char *block;

    if (posix_memalign((void **) &block, block_size, block_size) == 0) {
      const clock_t start = clock();
      read(fd, block, block_size);

      if (block[0] != 0x18 || block[1] != 0x10) {
        close(fd);
        free(block);
        write_log(LOG_ERR, "Invalid headers for database file, file is closed.");
        return false;
      }

      memcpy(server_age, block + 2, 8);

      const uint16_t filled_block_size = get_authorization_from_file(fd, block, block_size);
      const size_t data_count = get_all_data_from_file(conf, fd, file_size, block, block_size, filled_block_size);
      write_log(LOG_INFO,
        "Read database file in %.3f seconds. Loaded password count: %u, loaded data count: %d",
        ((float) clock() - start) / CLOCKS_PER_SEC, get_password_count(), data_count
      );

      free(block);
    }
  } else {
    set_main_database(create_database(CREATE_STRING(conf->database_name, strlen(conf->database_name))));
    write_log(LOG_INFO, "Database file is empty, loaded password and data count: 0");
    *server_age = 0;
  }

  return true;
}

const bool close_database_fd() {
  while (saving) {
    usleep(100);
  }

  lockf(fd, F_ULOCK, 0);
  return (close(fd) == 0);
}

static inline off_t get_value_size(const enum TellyTypes type, void *value) {
  switch (type) {
    case TELLY_NULL:
      return 0;

    case TELLY_INT: {
      mpz_t *val = value;
      const uint8_t byte_count = ((mpz_sizeinbase(*val, 2) + 7) / 8);

      return (byte_count + 1);
    }

    case TELLY_DOUBLE:
      // TODO
      return -1;

    case TELLY_STR: {
      const string_t *string = value;
      const uint8_t byte_count = get_byte_count(string->len);

      return (byte_count + string->len);
    }

    case TELLY_BOOL:
      return 1;

    case TELLY_HASHTABLE: {
      const struct HashTable *table = value;
      off_t length = 5;

      for (uint32_t i = 0; i < table->size.capacity; ++i) {
        struct HashTableField *field = table->fields[i];

        if (field) {
          length += (1 + get_value_size(TELLY_STR, &field->name) + get_value_size(field->type, field->value));
        }
      }

      return length;
    }

    case TELLY_LIST: {
      const struct List *list = value;
      struct ListNode *node = list->begin;
      off_t length = 4;

      while (node) {
        length += (1 + get_value_size(node->type, node->value));
        node = node->next;
      }

      return length;
    }

    default:
      return 0;
  }
}

static inline void generate_integer_value(char **data, off_t *len, mpz_t *number) {
  const bool negative = (mpz_sgn(*number) == -1);
  const uint8_t byte_count = ((mpz_sizeinbase(*number, 2) + 7) / 8);

  (*data)[*len] = byte_count | (negative << 7);
  char *hex = mpz_get_str(NULL, 16, *number);
  const bool is_even = mpz_tstbit(*number, ((byte_count - negative) * 2) - 1);

  int i = 0;

  for (uint8_t i = 0; i < byte_count; i++) {
    char byte[3];

    if (i == 0 && !is_even) {
      byte[0] = '0';
      byte[1] = hex[0 + negative];
      byte[2] = '\0';
    } else {
      const uint8_t str_index = ((is_even ? (i * 2) : ((i * 2) - 1)) + negative);
      byte[0] = hex[str_index];
      byte[1] = hex[str_index + 1];
      byte[2] = '\0';
    }

    const uint64_t value = strtol(byte, NULL, 16);
    (*data)[*len += 1] = value;
  }

  *len += 1;
}

static inline uint32_t generate_string_value(char **data, off_t *len, const string_t *string) {
  const uint8_t byte_count = get_byte_count(string->len >> 6);
  const uint8_t first = (byte_count << 6) | (string->len & 0b111111);
  const uint32_t length_in_bytes = string->len >> 6;

  (*data)[*len] = first;
  memcpy(*data + (*len += 1), &length_in_bytes, byte_count);
  memcpy(*data + (*len += byte_count), string->value, string->len);
  *len += string->len;

  return (1 + byte_count + string->len);
}

static void generate_boolean_value(char **data, off_t *len, const bool *boolean) {
  (*data)[*len] = *boolean;
  *len += 1;
}

static inline off_t generate_value(char **data, struct KVPair *kv) {
  off_t len = 0;

  generate_string_value(data, &len, &kv->key);
  (*data)[len] = kv->type;
  len += 1;

  switch (kv->type) {
    case TELLY_NULL:
      break;

    case TELLY_INT:
      generate_integer_value(data, &len, kv->value);
      break;

    case TELLY_DOUBLE:
      // generate_number_value(data, &len, kv->value);
      break;

    case TELLY_STR:
      generate_string_value(data, &len, kv->value);
      break;

    case TELLY_BOOL:
      generate_boolean_value(data, &len, kv->value);
      break;

    case TELLY_HASHTABLE: {
      struct HashTable *table = kv->value;
      memcpy(*data + len, &table->size.capacity, 4);
      len += 4;

      for (uint32_t i = 0; i < table->size.capacity; ++i) {
        struct HashTableField *field = table->fields[i];

        if (field) {
          (*data)[len] = field->type;
          len += 1;

          generate_string_value(data, &len, &field->name);

          switch (field->type) {
            case TELLY_INT:
              generate_integer_value(data, &len, field->value);
              break;

            case TELLY_DOUBLE:
              // generate_integer_value(data, &len, field->value);
              break;

            case TELLY_STR:
              generate_string_value(data, &len, field->value);
              break;

            case TELLY_BOOL:
              generate_boolean_value(data, &len, field->value);
              break;

            default:
              break;
          }
        }
      }

      (*data)[len] = 0x17;
      len += 1;

      break;
    }

    case TELLY_LIST: {
      struct List *list = kv->value;
      memcpy(*data + len, &list->size, 4);
      len += 4;

      struct ListNode *node = list->begin;

      while (node) {
        (*data)[len] = node->type;
        len += 1;

        switch (node->type) {
          case TELLY_INT:
            generate_integer_value(data, &len, node->value);
            break;

          case TELLY_DOUBLE:
            // generate_integer_value(data, &len, field->value);
            break;

          case TELLY_STR:
            generate_string_value(data, &len, node->value);
            break;

          case TELLY_BOOL:
            generate_boolean_value(data, &len, node->value);
            break;

          default:
            break;
        }

        node = node->next;
      }

      break;
    }

    default:
      len = 0;
      break;
  }

  return len;
}

static inline void generate_headers(char *headers, const uint32_t server_age) {
  headers[0] = 0x18;
  headers[1] = 0x10;
  memcpy(headers + 2, &server_age, sizeof(uint32_t));
}

const bool save_data(const uint32_t server_age) {
  if (saving) {
    return false;
  }

  saving = true;
  lseek(fd, 0, SEEK_SET);

  char *block;
  
  if (posix_memalign((void **) &block, block_size, block_size) != 0) {
    return false;
  }

  memset(block, 0, block_size);

  // length represents filled block size
  // total represents total calculated file size
  off_t length, total = 0;
  generate_headers(block, server_age);

  {
    struct Password **passwords = get_passwords();
    const uint32_t password_count = get_password_count();
    const uint8_t password_count_byte_count = (password_count != 0) ? (log2(password_count) + 1) : 0;
    length = 11 + password_count_byte_count;

    block[10] = password_count_byte_count;
    memcpy(block + 11, &password_count, password_count_byte_count);

    if (password_count == 0) {
      total += length;
    }

    for (uint32_t i = 0; i < password_count; ++i) {
      struct Password *password = passwords[i];
      const uint32_t new_length = (length + 49);

      if (new_length > block_size) {
        const uint32_t allowed = (new_length - block_size);
        memcpy(block + length, password->data, allowed);
        write(fd, block, block_size);

        const uint32_t remaining = (48 - allowed); // remaining byte count except permissions
        memcpy(block, password->data, remaining);
        block[remaining] = password->permissions;
        length = (remaining + 1);
        total += block_size + length;
      } else {
        memcpy(block + length, password->data, 48);
        block[length + 48] = password->permissions;
        length = new_length;
        total += length;
      }
    }
  }

  {
    struct LinkedListNode *node = get_database_node();

    while (node) {
      struct Database *database = node->data;
      uint32_t size = 0;
      struct BTreeValue **values = get_values_from_btree(database->cache, &size);

      if (values == NULL && database->cache->size != 0) {
        write_log(LOG_ERR, "Cannot collect data to save to database file, out of memory.");
        free(block);
        return false;
      }

      memcpy(block + length, &size, 4);
      length += 4;
      total += generate_string_value(&block, &length, &database->name) + 4;

      uint64_t data_size = 0;

      for (uint32_t i = 0; i < size; ++i) {
        struct KVPair *kv = values[i]->data;
        uint64_t kv_size = (1 + get_value_size(TELLY_STR, &kv->key) + get_value_size(kv->type, kv->value));
        data_size = fmax(data_size, kv_size);
      }

      char *data = malloc(data_size);

      for (uint32_t i = 0; i < size; ++i) {
        struct KVPair *kv = values[i]->data;
        const off_t kv_size = generate_value(&data, kv);

        const uint32_t block_count = ((length + kv_size + block_size - 1) / block_size);

        if (block_count != 1) {
          off_t remaining = kv_size;
          const uint16_t complete = (block_size - length);

          memcpy(block + length, data, complete);
          write(fd, block, block_size);
          remaining -= complete;

          if (remaining > block_size) {
            do {
              memcpy(block, data + (kv_size - remaining), block_size);
              write(fd, block, block_size);
              remaining -= block_size;
            } while (remaining > block_size);
          }

          length = remaining;
          memcpy(block, data + (kv_size - remaining), length);
        } else {
          memcpy(block + length, data, kv_size);
          length += kv_size;
        }

        total += kv_size;
      }

      if (values) {
        free(values);
      }

      free(data);
      node = node->next;
    }
  }

  if (length != block_size) write(fd, block, block_size);
  ftruncate(fd, total);
  free(block);

  saving = false;
  return true;
}

void *save_thread(void *arg) {
  const uint64_t *server_age = arg;
  save_data(*server_age);

  pthread_exit(NULL);
}

const bool bg_save(const uint32_t server_age) {
  if (saving) {
    return false;
  }

  pthread_t thread;
  pthread_create(&thread, NULL, save_thread, (uint32_t *) &server_age);
  pthread_detach(thread);

  return true;
}
