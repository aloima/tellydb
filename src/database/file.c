#include "../../headers/telly.h"

#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

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

uint32_t generate_data_content(char **line, struct KVPair *pair) {
  uint32_t len;

  switch (pair->type) {
    case TELLY_STR:
      len = pair->key->len + pair->value->string.len + 3;
      *line = malloc(len + 1);

      memcpy(*line, pair->key->value, pair->key->len);
      (*line)[pair->key->len] = 0x1D;
      (*line)[pair->key->len + 1] = TELLY_STR;
      memcpy(*line + pair->key->len + 2, pair->value->string.value, pair->value->string.len);
      (*line)[pair->key->len + 2 + pair->value->string.len] = 0x1E;
      (*line)[pair->key->len + 3 + pair->value->string.len] = 0x00;
      break;

    case TELLY_INT: {
      const uint32_t bit_count = log2(pair->value->integer) + 1;
      const uint32_t byte_count = (bit_count / 8) + 1;

      len = pair->key->len + byte_count + 3;
      *line = malloc(len + 1);

      memcpy(*line, pair->key->value, pair->key->len);
      (*line)[pair->key->len] = 0x1D;
      (*line)[pair->key->len + 1] = TELLY_INT;

      for (uint32_t i = 1; i <= byte_count; ++i) {
        (*line)[pair->key->len + 1 + i] = (pair->value->integer >> (8 * (byte_count - i))) & 0xFF;
      }

      (*line)[pair->key->len + 2 + byte_count] = 0x1E;
      (*line)[pair->key->len + 3 + byte_count] = 0x00;
      break;
    }

    case TELLY_BOOL:
      len = pair->key->len + 4;
      *line = malloc(len + 1);

      memcpy(*line, pair->key->value, pair->key->len);
      (*line)[pair->key->len] = 0x1D;
      (*line)[pair->key->len + 1] = TELLY_BOOL;
      (*line)[pair->key->len + 2] = pair->value->boolean;
      (*line)[pair->key->len + 3] = 0x1E;
      (*line)[pair->key->len + 4] = 0x00;
      break;

    case TELLY_NULL:
      len = pair->key->len + 3;
      *line = malloc(len + 1);

      memcpy(*line, pair->key->value, pair->key->len);
      (*line)[pair->key->len] = 0x1D;
      (*line)[pair->key->len + 1] = TELLY_NULL;
      (*line)[pair->key->len + 2] = 0x1E;
      (*line)[pair->key->len + 3] = 0x00;
      break;

    default:
      len = 0;
  }

  return len;
}

void save_data() {
  struct BTree *cache = get_cache();

  struct KVPair **pairs = get_sorted_kvs_from_btree(cache);
  const uint32_t size = cache->size;
  sort_kvs_by_pos(pairs, size);

  uint32_t file_size = lseek(fd, 0, SEEK_END);
  int32_t diff = 0;

  for (uint32_t i = 0; i < size; ++i) {
    struct KVPair *pair = pairs[i];

    char *line = NULL;
    const uint32_t line_len = generate_data_content(&line, pair);

    if (line_len != 0) {
      if (pair->pos != -1) {
        uint32_t end_pos = pair->pos + 1;

        lseek(fd, pair->pos + diff, SEEK_SET);
        while (read_char(fd) != 0x1E) end_pos += 1;

        const uint32_t line_len_in_file = end_pos - pair->pos;

        if (line_len_in_file != line_len) {
          char *buf = malloc(file_size - end_pos);
          read(fd, buf, file_size - end_pos);
          lseek(fd, pair->pos + diff, SEEK_SET);
          write(fd, line, line_len);
          write(fd, buf, file_size - end_pos);

          free(buf);

          diff += line_len - line_len_in_file;
        } else {
          lseek(fd, pair->pos + diff, SEEK_SET);
          write(fd, line, line_len);
        }
      } else {
        lseek(fd, 0, SEEK_END);
        write(fd, line, line_len);
        file_size += line_len;
      }

      free(line);
    }
  }

  ftruncate(fd, file_size + diff);
  free(pairs);
}

char read_char(int fd) {
  char c;
  return (read(fd, &c, 1) == 0 ? EOF : c);
}
