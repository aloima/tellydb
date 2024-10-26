#include "../../headers/database.h"
#include "../../headers/btree.h"
#include "../../headers/hashtable.h"
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

static off_t generate_value(char **line, struct KVPair *kv) {
  off_t len;

  switch (kv->type) {
    case TELLY_NULL:
      len = 1;
      *line = malloc(len);

      (*line)[0] = TELLY_NULL;
      break;

    case TELLY_NUM: {
      const long *number = kv->value;
      const uint32_t bit_count = log2(*number) + 1;
      const uint32_t byte_count = (bit_count / 8) + 1;

      len = byte_count + 2;
      *line = malloc(len);

      (*line)[0] = TELLY_NUM;
      (*line)[1] = byte_count;
      memcpy(*line + 2, number, byte_count);
      break;
    }

    case TELLY_STR: {
      const string_t *string = kv->value;

      const uint8_t bit_count = log2(string->len) + 1;
      const uint8_t byte_count = ceil((float) (bit_count - 6) / 8);
      const uint8_t first = (byte_count << 6) | (string->len & 0b111111);
      const uint32_t length_in_bytes = string->len >> 6;

      len = string->len + byte_count + 2;
      *line = malloc(len);

      (*line)[0] = TELLY_STR;
      (*line)[1] = first;
      memcpy(*line + 2, &length_in_bytes, byte_count);

      memcpy(*line + byte_count + 2, string->value, string->len);
      break;
    }

    case TELLY_BOOL:
      len = 2;
      *line = malloc(len);

      (*line)[0] = TELLY_BOOL;
      (*line)[1] = *((bool *) kv->value);
      break;

    case TELLY_HASHTABLE: {
      struct HashTable *table = kv->value;

      len = 5;
      *line = malloc(len);

      (*line)[0] = TELLY_HASHTABLE;
      memcpy(*line + 1, &table->size.allocated, sizeof(uint32_t));

      off_t at = len;

      for (uint32_t i = 0; i < table->size.allocated; ++i) {
        struct FVPair *fv = table->fvs[i];

        while (fv) {
          const string_t name = fv->name;
          const uint8_t n_bit_count = log2(name.len) + 1;
          const uint8_t n_byte_count = ceil((float) (n_bit_count - 6) / 8);
          const uint8_t n_first = (n_byte_count << 6) | (name.len & 0b111111);
          const uint32_t n_length_in_bytes = name.len >> 6;

          switch (fv->type) {
            case TELLY_NULL:
              len += 2 + n_byte_count + name.len;
              *line = realloc(*line, len);

              (*line)[at] = TELLY_NULL;
              (*line)[at += 1] = n_first;
              memcpy(*line + (at += 1), &n_length_in_bytes, n_byte_count);
              memcpy(*line + (at += n_byte_count), name.value, name.len);
              at += name.len;
              break;

            case TELLY_NUM: {
              const long *number = fv->value;
              const uint32_t bit_count = log2(*number) + 1;
              const uint32_t byte_count = (bit_count / 8) + 1;

              len += 3 + n_byte_count + name.len + byte_count;
              *line = realloc(*line, len);

              (*line)[at] = TELLY_NUM;
              (*line)[at += 1] = n_first;
              memcpy(*line + (at += 1), &n_length_in_bytes, n_byte_count);
              memcpy(*line + (at += n_byte_count), name.value, name.len);

              (*line)[at += name.len] = byte_count;
              memcpy(*line + (at += 1), number, byte_count);
              at += byte_count;
              break;
            }

            case TELLY_STR: {
              string_t *string = fv->value;

              const uint8_t bit_count = log2(string->len) + 1;
              const uint8_t byte_count = ceil((float) (bit_count - 6) / 8);
              const uint8_t first = (byte_count << 6) | (string->len & 0b111111);
              const uint32_t length_in_bytes = string->len >> 6;

              len += 3 + n_byte_count + name.len + byte_count + string->len;
              *line = realloc(*line, len);

              (*line)[at] = TELLY_STR;
              (*line)[at += 1] = n_first;
              memcpy(*line + (at += 1), &n_length_in_bytes, n_byte_count);
              memcpy(*line + (at += n_byte_count), name.value, name.len);

              (*line)[at += name.len] = first;
              memcpy(*line + (at += 1), &length_in_bytes, byte_count);
              memcpy(*line + (at += byte_count), string->value, string->len);
              at += string->len;
              break;
            }

            case TELLY_BOOL:
              len += 3 + n_byte_count + name.len;
              *line = realloc(*line, len);

              (*line)[at] = TELLY_BOOL;
              (*line)[at += 1] = n_first;
              memcpy(*line + (at += 1), &n_length_in_bytes, n_byte_count);
              memcpy(*line + (at += n_byte_count), name.value, name.len);

              (*line)[at += name.len] = *((bool *) fv->value);
              at += 1;
              break;

            default:
              break;
          }

          fv = fv->next;
        }
      }

      len += 1;
      *line = realloc(*line, len);
      (*line)[at] = 0x17;
      break;
    }

    case TELLY_LIST: {
      struct List *list = kv->value;
      struct ListNode *node = list->begin;

      len = 5;
      *line = malloc(len);

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
    const off_t line_len = generate_value(&line, kv);

    if (line_len != 0) {
      if (kv->pos.start_at != -1) {
        const off_t line_len_in_file = kv->pos.end_at - (kv->pos.start_at - 1);

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
          kv->pos.end_at += diff;
        } else {
          lseek(fd, kv->pos.start_at + diff - 1, SEEK_SET);
          write(fd, line, line_len);
        }
      } else {
        kv->pos.start_at = lseek(fd, 0, SEEK_END);

        const string_t key = kv->key;

        const uint8_t bit_count = log2(key.len) + 1;
        const uint8_t byte_count = ceil((float) (bit_count - 6) / 8);
        const uint8_t first = (byte_count << 6) | (key.len & 0b111111);
        const uint32_t length_in_bytes = key.len >> 6;

        const uint32_t length_specifier_len = byte_count + 1;
        const uint32_t key_part_len = key.len + length_specifier_len;

        kv->pos.end_at = kv->pos.start_at + key_part_len + line_len;
        kv->pos.start_at += key_part_len + 1;

        const uint32_t buf_len = key_part_len + line_len;
        char buf[buf_len];

        buf[0] = first;
        memcpy(buf + 1, &length_in_bytes, byte_count);
        memcpy(buf + length_specifier_len, key.value, key.len);
        memcpy(buf + length_specifier_len + key.len, line, line_len);

        write(fd, buf, buf_len);
        file_size += buf_len;
      }

      free(line);
    }
  }

  ftruncate(fd, file_size + diff);
  free(kvs);
}
