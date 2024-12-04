#include "../../headers/database.h"
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
  if ((fd = open_file(filename, O_LARGEFILE)) != -1) {
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
  }

  return true;
}

uint16_t get_block_size() {
  return block_size;
}

int get_database_fd() {
  return fd;
}

void close_database_fd() {
  while (saving) usleep(100);
  close(fd);
}

static off64_t generate_value(char **data, struct KVPair *kv) {
  off64_t len;

  {
    string_t key = kv->key;

    const uint8_t bit_count = log2(key.len) + 1;
    const uint8_t byte_count = ceil((float) (bit_count - 6) / 8);
    const uint8_t first = (byte_count << 6) | (key.len & 0b111111);
    const uint32_t length_in_bytes = key.len >> 6;

    len = key.len + byte_count + 1;
    *data = realloc(*data, len);

    (*data)[0] = first;
    memcpy(*data + 1, &length_in_bytes, byte_count);
    memcpy(*data + byte_count + 1, key.value, key.len);
  }

  switch (kv->type) {
    case TELLY_NULL:
      *data = realloc(*data, len + 1);
      (*data)[len] = TELLY_NULL;

      len += 1;
      break;

    case TELLY_NUM: {
      const long *number = kv->value;
      const uint32_t bit_count = log2(*number) + 1;
      const uint32_t byte_count = (bit_count / 8) + 1;

      *data = realloc(*data, len + byte_count + 2);
      (*data)[len] = TELLY_NUM;
      (*data)[len += 1] = byte_count;
      memcpy(*data + (len += 1), number, byte_count);

      len += byte_count;
      break;
    }

    case TELLY_STR: {
      const string_t *string = kv->value;

      const uint8_t bit_count = log2(string->len) + 1;
      const uint8_t byte_count = ceil((float) (bit_count - 6) / 8);
      const uint8_t first = (byte_count << 6) | (string->len & 0b111111);
      const uint32_t length_in_bytes = string->len >> 6;

      *data = realloc(*data, len + string->len + byte_count + 2);
      (*data)[len] = TELLY_STR;
      (*data)[len += 1] = first;
      memcpy(*data + (len += 1), &length_in_bytes, byte_count);
      memcpy(*data + (len += byte_count), string->value, string->len);

      len += string->len;
      break;
    }

    case TELLY_BOOL:
      *data = realloc(*data, len + 2);
      (*data)[len] = TELLY_BOOL;
      (*data)[len += 1] = *((bool *) kv->value);

      len += 1;
      break;

    default:
      len = 0;
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
      uint32_t size;
      struct BTree *cache = get_cache();
      struct BTreeValue **values = get_values_from_btree(cache, &size);

      char *data = malloc(1);

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
