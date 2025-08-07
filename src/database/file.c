#include <telly.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

static int fd = -1;
static bool saving = false;
static uint16_t block_size;

bool open_database_fd(struct Configuration *conf, uint32_t *server_age) {
  if ((fd = open_file(conf->data_file, 0)) == -1) return false;

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
      write_log(LOG_INFO, "Read database file in %.3f seconds. Loaded password count: %u, loaded data count: %d", ((float) clock() - start) / CLOCKS_PER_SEC, get_password_count(), data_count);

      free(block);
    }
  } else {
    set_main_database(create_database((string_t) {conf->database_name, strlen(conf->database_name)}));
    write_log(LOG_INFO, "Database file is empty, loaded password and data count: 0");
    *server_age = 0;
  }

  return true;
}

void close_database_fd() {
  while (saving) usleep(100);
  lockf(fd, F_ULOCK, 0);
  close(fd);
}

static off_t get_value_size(const enum TellyTypes type, void *value) {
  switch (type) {
    case TELLY_NULL:
      return 0;

    case TELLY_NUM: {
      const uint32_t bit_count = log2(*((long *) value)) + 1;
      return ((bit_count / 8) + 1);
    }

    case TELLY_STR: {
      const string_t *string = value;
      const uint8_t bit_count = log2(string->len) + 1;
      const uint8_t byte_count = ceil(((float) bit_count) / 8);

      return (byte_count + string->len);
    }

    case TELLY_BOOL:
      return 1;

    case TELLY_HASHTABLE: {
      const struct HashTable *table = value;
      off_t length = 5;

      for (uint32_t i = 0; i < table->size.allocated; ++i) {
        struct HashTableField *field = table->fields[i];

        while (field) {
          length += (1 + get_value_size(TELLY_STR, &field->name) + get_value_size(field->type, field->value));
          field = field->next;
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

static void generate_number_value(char **data, off_t *len, const long *number) {
  const uint32_t bit_count = log2(*number) + 1;
  const uint32_t byte_count = (bit_count / 8) + 1;

  (*data)[*len] = byte_count;
  memcpy(*data + (*len += 1), number, byte_count);
  *len += byte_count;
}

static uint32_t generate_string_value(char **data, off_t *len, const string_t *string) {
  const uint8_t bit_count = log2(string->len) + 1;
  const uint8_t byte_count = ceil((float) (bit_count - 6) / 8);
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

static off_t generate_value(char **data, struct KVPair *kv) {
  off_t len = 0;

  generate_string_value(data, &len, &kv->key);
  (*data)[len] = kv->type;
  len += 1;

  switch (kv->type) {
    case TELLY_NULL:
      break;

    case TELLY_NUM:
      generate_number_value(data, &len, kv->value);
      break;

    case TELLY_STR:
      generate_string_value(data, &len, kv->value);
      break;

    case TELLY_BOOL:
      generate_boolean_value(data, &len, kv->value);
      break;

    case TELLY_HASHTABLE: {
      struct HashTable *table = kv->value;
      memcpy(*data + len, &table->size.allocated, 4);
      len += 4;

      for (uint32_t i = 0; i < table->size.allocated; ++i) {
        struct HashTableField *field = table->fields[i];

        while (field) {
          (*data)[len] = field->type;
          len += 1;

          generate_string_value(data, &len, &field->name);

          switch (field->type) {
            case TELLY_NULL:
              break;

            case TELLY_NUM:
              generate_number_value(data, &len, field->value);
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

          field = field->next;
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
          case TELLY_NULL:
            break;

          case TELLY_NUM:
            generate_number_value(data, &len, node->value);
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

static void generate_headers(char *headers, const uint32_t server_age) {
  headers[0] = 0x18;
  headers[1] = 0x10;
  memcpy(headers + 2, &server_age, sizeof(uint32_t));
}

void save_data(const uint32_t server_age) {
  if (saving) return;
  saving = true;

  lseek(fd, 0, SEEK_SET);

  char *block;
  
  if (posix_memalign((void **) &block, block_size, block_size) == 0) {
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

      if (password_count == 0) total += length;

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
          break;
        }

        memcpy(block + length, &size, 4);
        length += 4;
        total += generate_string_value(&block, &length, &database->name);
        total += 4;

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
  
        if (values) free(values);
        free(data);

        node = node->next;
      }
    }

    if (length != block_size) write(fd, block, block_size);
    ftruncate(fd, total);
    free(block);
  }

  saving = false;
}

void *save_thread(void *arg) {
  const uint64_t *server_age = arg;
  save_data(*server_age);

  pthread_exit(NULL);
}

bool bg_save(const uint32_t server_age) {
  if (saving) return false;

  pthread_t thread;
  pthread_create(&thread, NULL, save_thread, (uint32_t *) &server_age);
  pthread_detach(thread);

  return true;
}
