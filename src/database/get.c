#include "../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

struct KVPair *get_data(char *key, __attribute__((unused)) struct Configuration *conf) {
  FILE *file = get_database_file();
  struct BTree *cache = get_cache();

  struct KVPair *data = find_kv_from_btree(cache, key);

  if (data == NULL) {
    if (file == NULL) {
      write_log(LOG_ERR, "Database file is not opened.");
      return NULL;
    }

    rewind(file);

    const uint32_t key_len = strlen(key);
    uint32_t key_at = 0;

    char c;

    while ((c = fgetc(file)) != EOF) {
      if (key_len == key_at) {
        if (c != 0x1D) {
          close_database_file();
          write_log(LOG_ERR, "Database file is corrupted.");
          return NULL;
        }

        const enum TellyTypes type = fgetc(file);
        const uint32_t pos = ftell(file) - key_len - 2;

        switch (type) {
          case TELLY_NULL:
            data = insert_kv_to_btree(cache, key, NULL, type);

            if (fgetc(file) != 0x1E) {
              close_database_file();
              write_log(LOG_ERR, "Database file is corrupted.");
              return NULL;
            }

            data->pos = pos;
            break;

          case TELLY_INT: {
            char *value = malloc(33);
            uint32_t len = 0;

            while ((value[len] = fgetc(file)) != 0x1E) {
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

            while ((value[len] = fgetc(file)) != 0x1E) {
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
            c = fgetc(file);

            if (fgetc(file) != 0x1E) {
              close_database_file();
              write_log(LOG_ERR, "Database file is corrupted.");
              return NULL;
            }

            data = insert_kv_to_btree(cache, key, &c, type);
            data->pos = pos;
            break;

          default:
            close_database_file();
            write_log(LOG_ERR, "Database file is corrupted.");
            break;
        }

        return data;
      } else if (key[key_at] == c) {
        key_at += 1;
      } else {
        while ((c = fgetc(file)) != 0x1E) {
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
