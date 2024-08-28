#include "../../headers/telly.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

static struct BTree *cache = NULL;
static FILE *file = NULL;

void create_cache() {
  cache = create_btree(3);
}

void free_cache() {
  free_btree(cache);
}

void open_database_file(const char *filename) {
  file = fopen(filename, "a+");
}

void close_database_file() {
  fclose(file);
}

struct KVPair *get_data(char *key, struct Configuration *conf) {
  struct KVPair *data = find_kv_from_btree(cache, key);

  if (data == NULL) {
    FILE *file = fopen(conf->data_file, "r");
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

void set_data(struct KVPair pair) {
  struct KVPair *data = find_kv_from_btree(cache, pair.key.value);

  if (data != NULL) {
    if (data->type == TELLY_STR && pair.type != TELLY_STR) {
      free(data->value.string.value);
    }

    switch (pair.type) {
      case TELLY_STR:
        set_string(&data->value.string, pair.value.string.value, pair.value.string.len);
        break;

      case TELLY_INT:
        data->value.integer = pair.value.integer;
        break;

      case TELLY_BOOL:
        data->value.boolean = pair.value.boolean;
        break;

      case TELLY_NULL:
        data->value.null = NULL;
        break;
    }

    data->type = pair.type;
  } else {
    void *value = get_kv_val(&pair, pair.type);
    insert_kv_to_btree(cache, pair.key.value, value, pair.type);
  }
}

void save_data() {
  struct KVPair **pairs = get_kvs_from_btree(cache);
  const uint32_t size = get_total_size_of_node(cache->root);

  for (uint32_t i = 0; i < size; ++i) {
    struct KVPair *pair = pairs[i];
    char *line = NULL;

    switch (pair->type) {
      case TELLY_STR:
        line = malloc(pair->key.len + pair->value.string.len + 3);
        memcpy(line, pair->key.value, pair->key.len);
        line[pair->key.len] = 0x1D;
        line[pair->key.len + 1] = TELLY_INT;
        memcpy(line + pair->key.len + 2, pair->value.string.value, pair->value.string.len + 1);
        break;

      case TELLY_INT: {
        const uint32_t bit_count = log2(pair->value.integer) + 1;
        const uint32_t byte_count = (bit_count / 8) + 1;
        line = malloc(pair->key.len + byte_count + 4);

        memcpy(line, pair->key.value, pair->key.len);
        line[pair->key.len] = 0x1D;
        line[pair->key.len + 1] = TELLY_INT;

        for (uint32_t i = 1; i <= byte_count; ++i) {
          line[pair->key.len + 1 + i] = (pair->value.integer >> (8 * (byte_count - i))) & 0xFF;
        }

        line[pair->key.len + 2 + byte_count] = 0x1E;
        line[pair->key.len + 3 + byte_count] = 0x00;
        break;
      }

      case TELLY_BOOL:
        line = malloc(pair->key.len + 5);
        memcpy(line, pair->key.value, pair->key.len);
        line[pair->key.len] = 0x1D;
        line[pair->key.len + 1] = TELLY_BOOL;
        line[pair->key.len + 2] = pair->value.boolean;
        line[pair->key.len + 3] = 0x1E;
        line[pair->key.len + 4] = 0x00;
        break;

      case TELLY_NULL:
        line = malloc(pair->key.len + 4);
        memcpy(line, pair->key.value, pair->key.len);
        line[pair->key.len] = 0x1D;
        line[pair->key.len + 1] = TELLY_NULL;
        line[pair->key.len + 2] = 0x1E;
        line[pair->key.len + 3] = 0x00;
        break;
    }

    fputs(line, file);
    free(line);
  }
}
