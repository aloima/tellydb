#include "../../headers/telly.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static FILE *file = NULL;

void open_database_file(const char *filename) {
  file = fopen(filename, "a+");
}

FILE *get_database_file() {
  return file;
}

void close_database_file() {
  fclose(file);
}

void save_data() {
  struct BTree *cache = get_cache();

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
