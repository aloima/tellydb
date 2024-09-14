#include "../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <unistd.h>

struct KVPair *get_data(char *key, __attribute__((unused)) struct Configuration *conf) {
  const int fd = get_database_fd();
  struct BTree *cache = get_cache();

  struct KVPair *data = find_kv_from_btree(cache, key);

  if (data == NULL) {
    if (fd == -1) {
      write_log(LOG_ERR, "Database file is not opened.");
      return NULL;
    }

    lseek(fd, 0, SEEK_SET);

    const uint32_t key_len = strlen(key);
    uint32_t key_at = 0;

    char c;

    while (read(fd, &c, 1) != 0) {
      if (key_len == key_at) {
        if (c != 0x1D) {
          close_database_fd();
          write_log(LOG_ERR, "Database file is corrupted.");
          return NULL;
        }

        const enum TellyTypes type = read_char(fd);
        const uint32_t pos = lseek(fd, 0, SEEK_CUR) - key_len - 2;

        switch (type) {
          case TELLY_NULL:
            data = insert_kv_to_btree(cache, key, NULL, type);

            if (read_char(fd) != 0x1E) {
              close_database_fd();
              write_log(LOG_ERR, "Database file is corrupted.");
              return NULL;
            }

            data->pos = pos;
            break;

          case TELLY_INT: {
            char *value = malloc(33);
            uint32_t len = 0;

            while ((value[len] = read_char(fd)) != 0x1E) {
              len += 1;

              if (len % 32 == 0) {
                value = realloc(value, len + 33);
              }
            }

            value[len] = 0x00;

            int res = 0;

            for (uint32_t i = 0; i < len; ++i) {
              res |= (value[i] << (8 * (len - i - 1)));
            }

            data = insert_kv_to_btree(cache, key, &res, TELLY_INT);
            data->pos = pos;
            free(value);
            break;
          }

          case TELLY_STR: {
            char *value = malloc(33);
            uint32_t len = 0;

            while ((value[len] = read_char(fd)) != 0x1E) {
              len += 1;

              if (len % 32 == 0) {
                value = realloc(value, len + 33);
              }
            }

            value[len] = 0x00;
            data = insert_kv_to_btree(cache, key, value, TELLY_STR);
            data->pos = pos;
            free(value);
            break;
          }

          case TELLY_BOOL:
            c = read_char(fd);

            if (c == EOF || read_char(fd) != 0x1E) {
              close_database_fd();
              write_log(LOG_ERR, "Database file is corrupted.");
              return NULL;
            }

            data = insert_kv_to_btree(cache, key, &c, type);
            data->pos = pos;
            break;

          default:
            close_database_fd();
            write_log(LOG_ERR, "Database file is corrupted.");
            break;
        }

        return data;
      } else if (key[key_at] == c) {
        key_at += 1;
      } else {
        while ((c = read_char(fd)) != 0x1E) {
          if (c == EOF) {
            return NULL;
          }
        }

        key_at = 0;
      }
    }

    return NULL;
  } else {
    return data;
  }
}
