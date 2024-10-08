#include "../../headers/database.h"
#include "../../headers/btree.h"

#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

static int fd = -1;

void open_database_fd(const char *filename) {
  #if defined(__linux__)
    fd = open(filename, O_RDWR | O_CREAT | O_DIRECT, S_IRWXU);
  #elif defined(__APPLE__)
    fd = open(filename, O_RDWR | O_CREAT, S_IRWXU);

    if (fcntl(fd, F_NOCACHE, 1) == -1) {
      write_log(LOG_ERR, "Cannot deactive file caching for database file.");
    }
  #endif
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
    case TELLY_STR:
      len = kv->value->string.len + 2;
      *line = malloc(len + 1);

      (*line)[0] = TELLY_STR;
      memcpy(*line + 1, kv->value->string.value, kv->value->string.len);
      (*line)[1 + kv->value->string.len] = 0x1E;
      (*line)[2 + kv->value->string.len] = 0x00;
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

    case TELLY_BOOL:
      len = 3;
      *line = malloc(len + 1);

      (*line)[0] = TELLY_BOOL;
      (*line)[1] = kv->value->boolean;
      (*line)[2] = 0x1E;
      (*line)[3] = 0x00;
      break;

    case TELLY_NULL:
      len = 2;
      *line = malloc(len + 1);

      (*line)[0] = TELLY_NULL;
      (*line)[1] = 0x1E;
      (*line)[2] = 0x00;
      break;

    default:
      len = 0;
  }

  return len;
}

void save_data() {
  struct BTree *cache = get_cache();

  struct KVPair **kvs = get_sorted_kvs_from_btree(cache);
  const uint32_t size = cache->size;
  sort_kvs_by_pos(kvs, size);

  uint32_t file_size = lseek(fd, 0, SEEK_END);
  int32_t diff = 0;

  for (uint32_t i = 0; i < size; ++i) {
    struct KVPair *kv = kvs[i];

    char *line = NULL;
    const uint32_t line_len = generate_value(&line, kv);

    if (line_len != 0) {
      if (kv->pos.start_at != -1) {
        const uint32_t line_len_in_file = kv->pos.end_at - kv->pos.start_at;

        if (line_len_in_file != line_len) {
          const uint64_t n = file_size - kv->pos.end_at;
          char *buf = malloc(n);
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

        const uint32_t buf_len = kv->key->len + line_len + 1;
        char buf[buf_len + 1];
        memcpy(buf, kv->key->value, kv->key->len);
        buf[kv->key->len] = 0x1D;
        memcpy(buf + kv->key->len + 1, line, line_len);

        write(fd, buf, buf_len);
        file_size += buf_len;
      }

      free(line);
    }
  }

  ftruncate(fd, file_size + diff);
  free(kvs);
}

char read_char(int fd) {
  char c;
  return (read(fd, &c, 1) == 0 ? EOF : c);
}
