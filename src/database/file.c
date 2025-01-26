#include "../../headers/database.h"
#include "../../headers/server.h"
#include "../../headers/hashtable.h"
#include "../../headers/utils.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

static int fd = -1;
static bool saving = false;
static uint16_t block_size;

bool open_database_fd(const char *filename, uint64_t *server_age) {
  if ((fd = open_file(filename, O_LARGEFILE)) == -1) return false;

  struct stat sostat;
  stat(filename, &sostat);

  const off64_t file_size = sostat.st_size;;
  block_size = sostat.st_blksize;

  if (file_size != 0) {
    char *block;

    if (posix_memalign((void **) &block, block_size, block_size) == 0) {
      read(fd, block, block_size);

      if (block[0] != 0x18 || block[1] != 0x10) {
        close(fd);
        free(block);
        write_log(LOG_ERR, "Invalid headers for database file, file is closed.");
        return false;
      }

      memcpy(server_age, block + 2, 8);

      const uint16_t filled_block_size = get_authorization_from_file(fd, block, block_size);
      get_all_data_from_file(fd, file_size, block, block_size, filled_block_size);

      struct BTree *cache = get_cache();
      write_log(LOG_INFO, "Read database file. Loaded password count: %d, loaded data count: %d", get_password_count(), cache->size);

      free(block);
    }
  } else {
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

static off64_t get_value_size(const enum TellyTypes type, void *value) {
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
      off64_t length = 5;

      for (uint32_t i = 0; i < table->size.allocated; ++i) {
        struct FVPair *fv = table->fvs[i];

        while (fv) {
          length += (1 + get_value_size(TELLY_STR, &fv->name) + get_value_size(fv->type, value));
          fv = fv->next;
        }
      }

      return length;
    }

    case TELLY_LIST: {
      const struct List *list = value;
      struct ListNode *node = list->begin;
      off64_t length = 4;

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

static void generate_number_value(char **data, off64_t *len, const long *number) {
  const uint32_t bit_count = log2(*number) + 1;
  const uint32_t byte_count = (bit_count / 8) + 1;

  (*data)[*len] = byte_count;
  memcpy(*data + (*len += 1), number, byte_count);
  *len += byte_count;
}

static void generate_string_value(char **data, off64_t *len, const string_t *string) {
  const uint8_t bit_count = log2(string->len) + 1;
  const uint8_t byte_count = ceil((float) (bit_count - 6) / 8);
  const uint8_t first = (byte_count << 6) | (string->len & 0b111111);
  const uint32_t length_in_bytes = string->len >> 6;

  (*data)[*len] = first;
  memcpy(*data + (*len += 1), &length_in_bytes, byte_count);
  memcpy(*data + (*len += byte_count), string->value, string->len);
  *len += string->len;
}

static void generate_boolean_value(char **data, off64_t *len, const bool *boolean) {
  (*data)[*len] = *boolean;
  *len += 1;
}

static void generate_null_value(char **data, off64_t *len) {
  (*data)[*len] = TELLY_NULL;
  *len += 1;
}

static off64_t generate_value(char **data, struct KVPair *kv) {
  off64_t len = 0;

  generate_string_value(data, &len, &kv->key);
  (*data)[len] = kv->type;
  len += 1;

  switch (kv->type) {
    case TELLY_NULL:
      generate_null_value(data, &len);
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
        struct FVPair *fv = table->fvs[i];

        while (fv) {
          (*data)[len] = fv->type;
          len += 1;

          generate_string_value(data, &len, &fv->name);

          switch (fv->type) {
            case TELLY_NULL:
              generate_null_value(data, &len);
              break;

            case TELLY_NUM:
              generate_number_value(data, &len, fv->value);
              break;

            case TELLY_STR:
              generate_string_value(data, &len, fv->value);
              break;

            case TELLY_BOOL:
              generate_boolean_value(data, &len, fv->value);
              break;

            default:
              break;
          }

          fv = fv->next;
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
            generate_null_value(data, &len);
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

static void generate_headers(char *headers, const uint64_t server_age) {
  headers[0] = 0x18;
  headers[1] = 0x10;
  memcpy(headers + 2, &server_age, sizeof(uint64_t));
}

void save_data(const uint64_t server_age) {
  if (saving) return;
  saving = true;

  lseek(fd, 0, SEEK_SET);

  char *block;
  
  if (posix_memalign((void **) &block, block_size, block_size) == 0) {
    memset(block, 0, block_size);

    uint32_t length, total = 0;
    generate_headers(block, server_age);

    {
      struct Password **passwords = get_passwords();
      const uint32_t password_count = get_password_count();
      const uint8_t password_count_byte_count = (password_count != 0) ? (log2(password_count) + 1) : 0;
      length = 11 + password_count_byte_count;

      block[10] = password_count_byte_count;
      memcpy(block + 11, &password_count, password_count_byte_count);

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
          total += block_size;
          length = (remaining + 1);
        } else {
          memcpy(block + length, password->data, 48);
          block[length + 48] = password->permissions;
          length = new_length;
        }
      }
    }

    {
      uint32_t size = 0;
      struct BTree *cache = get_cache();
      struct BTreeValue **values = get_values_from_btree(cache, &size);

      if (values == NULL && cache->size != 0) {
        write_log(LOG_ERR, "Cannot collect data to save to database file, out of memory.");
        free(block);
      }

      off64_t memory_block_length = 0;

      for (uint32_t i = 0; i < size; ++i) {
        struct KVPair *kv = values[i]->data;
        memory_block_length += (1 + get_value_size(TELLY_STR, &kv->key) + get_value_size(kv->type, kv->value));
      }

      char *data = malloc(memory_block_length);

      for (uint32_t i = 0; i < size; ++i) {
        struct KVPair *kv = values[i]->data;
        const off64_t data_length = generate_value(&data, kv);

        const uint32_t block_count = ((length + data_length + block_size - 1) / block_size);

        if (block_count != 1) {
          off64_t remaining = data_length;
          const uint16_t complete = (block_size - length);

          memcpy(block + length, data, complete);
          write(fd, data, complete);
          remaining -= complete;

          if (remaining > block_size) {
            do {
              memcpy(block, data + (data_length - remaining), block_size);
              write(fd, block, block_size);
              remaining -= block_size;
            } while (remaining > block_size);

            length = (data_length - remaining);
          } else {
            length += (data_length - remaining);
          }
        } else {
          memcpy(block + length, data, data_length);
          length += data_length;
        }
      }

      total += length;
      if (values) free(values);
      free(data);
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

bool bg_save(uint64_t server_age) {
  if (saving) return false;

  pthread_t thread;
  pthread_create(&thread, NULL, save_thread, &server_age);
  pthread_detach(thread);

  return true;
}
