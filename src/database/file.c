#include "../../headers/database.h"
#include "../../headers/btree.h"
#include "../../headers/utils.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <fcntl.h>
#include <unistd.h>

static int fd = -1;

bool open_database_fd(const char *filename, uint64_t *server_age) {
  #if defined(__linux__)
    fd = open(filename, O_RDWR | O_CREAT | O_DIRECT, S_IRWXU);
  #elif defined(__APPLE__)
    fd = open(filename, O_RDWR | O_CREAT, S_IRWXU);

    if (fcntl(fd, F_NOCACHE, 1) == -1) {
      write_log(LOG_ERR, "Cannot deactive file caching for database file.");
      return false;
    }
  #endif

  if (fd == -1) {
    write_log(LOG_ERR, "Database file cannot be opened and created.");
    return false;
  }

  if (lseek(fd, 0, SEEK_END) != 0) {
    lseek(fd, 2, SEEK_SET);
    read(fd, server_age, sizeof(long));
  } else {
    *server_age = 0;
  }

  return true;
}

int get_database_fd() {
  return fd;
}

void close_database_fd() {
  close(fd);
}

static uint32_t generate_value(char **line, struct KVPair *kv) {
  uint32_t len;

  switch (kv->type) {
    case TELLY_NULL:
      len = 1;
      *line = malloc(len + 1);

      (*line)[0] = TELLY_NULL;
      (*line)[1] = 0x00;
      break;

    case TELLY_NUM: {
      const long *number = kv->value;
      const uint32_t bit_count = log2(*number) + 1;
      const uint32_t byte_count = (bit_count / 8) + 1;

      len = byte_count + 2;
      *line = malloc(len + 1);

      (*line)[0] = TELLY_NUM;
      (*line)[1] = byte_count;
      memcpy(*line + 2, number, byte_count);
      (*line)[2 + byte_count] = 0x00;
      break;
    }

    case TELLY_STR: {
      const string_t *string = kv->value;

      const uint8_t bit_count = log2(string->len) + 1;
      const uint8_t byte_count = ceil((float) (bit_count - 6) / 8);
      const uint8_t first = (byte_count << 6) | (string->len & 0b111111);
      const uint32_t length_in_bytes = string->len >> 6;

      len = string->len + byte_count + 2;
      *line = malloc(len + 1);

      (*line)[0] = TELLY_STR;
      (*line)[1] = first;
      memcpy(*line + 2, &length_in_bytes, byte_count);

      memcpy(*line + byte_count + 2, string->value, string->len);
      (*line)[2 + byte_count + string->len] = 0x00;
      break;
    }

    case TELLY_BOOL:
      len = 2;
      *line = malloc(len + 1);

      (*line)[0] = TELLY_BOOL;
      (*line)[1] = *((bool *) kv->value);
      (*line)[2] = 0x00;
      break;

    case TELLY_LIST: {
      struct List *list = kv->value;
      struct ListNode *node = list->begin;

      len = 5;
      *line = malloc(len + 1);

      (*line)[0] = TELLY_LIST;
      memcpy(*line + 1, &list->size, sizeof(uint32_t));

      while (node) {
        switch (node->type) {
          case TELLY_NULL:
            len += 1;
            *line = realloc(*line, len + 1);

            (*line)[len - 1] = TELLY_NULL;
            break;

          case TELLY_NUM: {
            const long *number = node->value;
            const uint32_t bit_count = log2(*number) + 1;
            const uint32_t byte_count = (bit_count / 8) + 1;

            len += byte_count + 2;
            *line = realloc(*line, len + 1);

            (*line)[len - byte_count - 2] = TELLY_NUM;
            (*line)[len - byte_count - 1] = byte_count;
            memcpy(*line + len - byte_count, number, byte_count);
            break;
          }

          case TELLY_STR: {
            string_t *string = node->value;

            const uint8_t bit_count = log2(string->len) + 1;
            const uint8_t byte_count = ceil((float) (bit_count - 6) / 8);
            const uint8_t first = (byte_count << 6) | (string->len & 0b111111);
            const uint32_t length_in_bytes = string->len >> 6;

            len += string->len + byte_count + 2;
            *line = realloc(*line, len + 1);

            (*line)[len - string->len - byte_count - 2] = TELLY_STR;
            (*line)[len - string->len - byte_count - 1] = first;
            memcpy(*line + len - string->len - byte_count, &length_in_bytes, byte_count);
            memcpy(*line + len - string->len, string->value, string->len);
            break;
          }

          case TELLY_BOOL:
            len += 2;
            *line = realloc(*line, len + 1);

            (*line)[len - 2] = TELLY_BOOL;
            (*line)[len - 1] = *((bool *) node->value);
            break;

          default:
            break;
        }

        node = node->next;
      }

      (*line)[len] = 0x00;
      break;
    }

    default:
      len = 0;
  }

  return len;
}

void save_data(const uint64_t server_age) {
  struct BTree *cache = get_cache();

  uint32_t size;
  struct KVPair **kvs = get_kvs_from_btree(cache, &size);
  sort_kvs_by_pos(kvs, size);

  uint32_t file_size = lseek(fd, 0, SEEK_END);
  int32_t diff = 0;

  if (file_size != 0) {
    uint8_t constants[2];
    lseek(fd, 0, SEEK_SET);

    if (read(fd, constants, 2) != 2 || constants[0] != 0x18 || constants[1] != 0x10 || file_size < 10) {
      write_log(LOG_ERR, "Cannot save data, invalid file headers");
      free(kvs);
      return;
    }

    write(fd, &server_age, sizeof(uint64_t));
  } else {
    uint8_t constants[2] = {0x18, 0x10};

    lseek(fd, 0, SEEK_SET);
    write(fd, constants, 2);
    write(fd, &server_age, sizeof(uint64_t));
    file_size += 10;
  }

  for (uint32_t i = 0; i < size; ++i) {
    struct KVPair *kv = kvs[i];

    char *line;
    const uint32_t line_len = generate_value(&line, kv);

    if (line_len != 0) {
      if (kv->pos.start_at != -1) {
        const uint32_t line_len_in_file = kv->pos.end_at - (kv->pos.start_at - 1);

        if (line_len_in_file != line_len) {
          const uint64_t n = file_size - kv->pos.end_at;
          char *buf = malloc(n);
          lseek(fd, kv->pos.end_at, SEEK_SET);
          read(fd, buf, n);
          lseek(fd, kv->pos.start_at + diff - 1, SEEK_SET);
          write(fd, line, line_len);
          write(fd, buf, n);

          free(buf);

          diff += line_len - line_len_in_file;
        } else {
          lseek(fd, kv->pos.start_at + diff - 1, SEEK_SET);
          write(fd, line, line_len);
        }
      } else {
        lseek(fd, 0, SEEK_END);

        const string_t key = kv->key;

        const uint8_t bit_count = log2(key.len) + 1;
        const uint8_t byte_count = ceil((float) (bit_count - 6) / 8);
        const uint8_t first = (byte_count << 6) | (key.len & 0b111111);
        const uint32_t length_in_bytes = key.len >> 6;

        const uint32_t length_specifier_length = byte_count + 1;
        const uint32_t buf_len = key.len + length_specifier_length + line_len;
        char buf[buf_len];

        buf[0] = first;
        memcpy(buf + 1, &length_in_bytes, byte_count);
        memcpy(buf + length_specifier_length, key.value, key.len);
        memcpy(buf + length_specifier_length + key.len, line, line_len);

        write(fd, buf, buf_len);
        file_size += buf_len;
      }

      free(line);
    }
  }

  ftruncate(fd, file_size + diff);
  free(kvs);
}
