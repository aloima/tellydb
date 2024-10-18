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
      len = 2;
      *line = malloc(len + 1);

      (*line)[0] = TELLY_NULL;
      (*line)[1] = 0x1E;
      (*line)[2] = 0x00;
      break;

    case TELLY_INT: {
      const uint32_t bit_count = log2(kv->value->integer) + 1;
      const uint32_t byte_count = (bit_count / 8) + 1;

      len = byte_count + 2;
      *line = malloc(len + 1);

      (*line)[0] = TELLY_INT;

      for (uint32_t i = 1; i <= byte_count; ++i) {
        (*line)[i] = (kv->value->integer >> (8 * (byte_count - i))) & 0xFF;
      }

      (*line)[1 + byte_count] = 0x1E;
      (*line)[2 + byte_count] = 0x00;
      break;
    }

    case TELLY_STR:
      len = kv->value->string.len + 2;
      *line = malloc(len + 1);

      (*line)[0] = TELLY_STR;
      memcpy(*line + 1, kv->value->string.value, kv->value->string.len);
      (*line)[1 + kv->value->string.len] = 0x1E;
      (*line)[2 + kv->value->string.len] = 0x00;
      break;

    case TELLY_BOOL:
      len = 3;
      *line = malloc(len + 1);

      (*line)[0] = TELLY_BOOL;
      (*line)[1] = kv->value->boolean;
      (*line)[2] = 0x1E;
      (*line)[3] = 0x00;
      break;

    case TELLY_LIST: {
      struct List *list = kv->value->list;
      struct ListNode *node = list->begin;

      len = 6;
      *line = malloc(len + 1);

      (*line)[0] = TELLY_LIST;
      (*line)[1] = (list->size >> 24) & 0xFF;
      (*line)[2] = (list->size >> 16) & 0xFF;
      (*line)[3] = (list->size >> 8) & 0xFF;
      (*line)[4] = list->size & 0xFF;

      while (node) {
        switch (node->type) {
          case TELLY_NULL:
            len += 2;
            *line = realloc(*line, len + 1);

            (*line)[len - 3] = TELLY_NULL;
            (*line)[len - 2] = 0x1F;
            break;

          case TELLY_INT: {
            const uint32_t bit_count = log2(node->value.integer) + 1;
            const uint32_t byte_count = (bit_count / 8) + 1;

            len += byte_count + 2;
            *line = realloc(*line, len + 1);

            (*line)[len - byte_count - 3] = TELLY_INT;

            for (uint32_t i = 1; i <= byte_count; ++i) {
              (*line)[len - i - 2] = (node->value.integer >> (8 * (byte_count - i))) & 0xFF;
            }

            (*line)[len - 2] = 0x1F;
            break;
          }

          case TELLY_STR: {
            string_t string = node->value.string;
            len += string.len + 2;
            *line = realloc(*line, len + 1);

            (*line)[len - string.len - 3] = TELLY_STR;
            memcpy(*line + len - string.len - 2, string.value, string.len);
            (*line)[len - 2] = 0x1F;
            break;
          }

          case TELLY_BOOL:
            len += 3;
            *line = realloc(*line, len + 1);

            (*line)[len - 4] = TELLY_BOOL;
            (*line)[len - 3] = kv->value->boolean;
            (*line)[len - 2] = 0x1F;
            break;

          default:
            break;
        }

        node = node->next;
      }

      (*line)[len - 1] = 0x1E;
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
        const uint32_t line_len_in_file = kv->pos.end_at - kv->pos.start_at;

        if (line_len_in_file != line_len) {
          const uint64_t n = file_size - kv->pos.end_at;
          char *buf = malloc(n);
          lseek(fd, kv->pos.end_at, SEEK_SET);
          read(fd, buf, n);
          lseek(fd, kv->pos.start_at + diff, SEEK_SET);
          write(fd, line, line_len);
          write(fd, buf, n);

          free(buf);

          diff += line_len - line_len_in_file;
        } else {
          lseek(fd, kv->pos.start_at + diff, SEEK_SET);
          write(fd, line, line_len);
        }
      } else {
        lseek(fd, 0, SEEK_END);

        const uint32_t buf_len = kv->key.len + line_len + 1;
        char buf[buf_len + 1];
        memcpy(buf, kv->key.value, kv->key.len);
        buf[kv->key.len] = 0x1D;
        memcpy(buf + kv->key.len + 1, line, line_len);

        write(fd, buf, buf_len);
        file_size += buf_len;
      }

      free(line);
    }
  }

  ftruncate(fd, file_size + diff);
  free(kvs);
}
