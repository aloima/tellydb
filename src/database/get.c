#include "../../headers/telly.h"

#include <stdio.h>
#include <stdlib.h>

struct KVPair *get_data(char *key, struct Configuration *conf) {
  FILE *file = get_database_file();
  struct BTree *cache = get_cache();

  struct KVPair *data = find_kv_from_btree(cache, key);

  if (data == NULL) {
    if (file == NULL) {
      write_log("Database file is not opened.", LOG_ERR, conf->allowed_log_levels);
      return NULL;
    }

    char *data_key = malloc(33);
    uint32_t data_key_len = 0;
    uint8_t type;

    char c;

    while ((c = fgetc(file)) != EOF) {
      if (c == key[0]) {
        data_key[data_key_len] = c;
        data_key_len += 1;

        while (true) {
          c = fgetc(file);

          if (c == EOF) {
            free(data_key);
            close_database_file();
            write_log("Database file is corrupted.", LOG_ERR, conf->allowed_log_levels);
            return NULL;
          } else if (c != 0x1D) {
            data_key[data_key_len] = c;
            data_key_len += 1;

            if (data_key_len % 32 == 0) data_key = realloc(data_key, data_key_len + 33);
          } else {
            data_key[data_key_len] = 0;
            type = fgetc(file);

            if (streq(data_key, key)) {
              free(data_key);

              if (type != TELLY_BOOL && type != TELLY_STR && type != TELLY_INT && type != TELLY_NULL) {
                close_database_file();
                write_log("Database file is corrupted.", LOG_ERR, conf->allowed_log_levels);
                return NULL;
              }

              switch (type) {
                case TELLY_NULL:
                  data = insert_kv_to_btree(cache, key, NULL, type);
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

                  value[len] = 0x0;

                  int res = 0;

                  for (uint32_t i = 0; i < len; ++i) {
                    res |= (value[i] << (8 * (len - i - 1)));
                  }

                  data = insert_kv_to_btree(cache, key, &res, TELLY_INT);
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
                  free(value);
                  break;
                }

                case TELLY_BOOL:
                  c = fgetc(file);
                  data = insert_kv_to_btree(cache, key, &c, type);

                  if (fgetc(file) != 0x1E) {
                    close_database_file();
                    write_log("Database file is corrupted.", LOG_ERR, conf->allowed_log_levels);
                    return NULL;
                  }

                  break;
              }

              return data;
            }
          }
        }
      } else {
        while (true) {
          c = fgetc(file);
          if (c == EOF || c == 0x1E) break;
        }
      }
    }

    return NULL;
  } else {
    return data;
  }
}
