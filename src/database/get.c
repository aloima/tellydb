#include "../../headers/database.h"
#include "../../headers/btree.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <unistd.h>

void get_all_keys() {
  const int fd = get_database_fd();
  char c;

  struct BTree *cache = get_cache();
  string_t key = {
    .value = malloc(33),
    .len = 0
  };

  while (read(fd, &c, 1) != 0) {
    if (c != 0x1D) {
      key.value[key.len] = c;
      key.len += 1;

      if (key.len % 32 == 0) {
        key.value = realloc(key.value, key.len + 33);
      }
    } else {
      const off_t start_at = lseek(fd, 0, SEEK_CUR);
      while (read(fd, &c, 1) == 1 && c != 0x1E);
      const off_t end_at = lseek(fd, 0, SEEK_CUR);

      key.value[key.len] = '\0';
      insert_kv_to_btree(cache, key, NULL, TELLY_UNSPECIFIED, start_at, end_at);
      free(key.value);

      key = (string_t) {
        .value = malloc(33),
        .len = 0
      };
    }
  }

  free(key.value);
}

struct KVPair *get_data(const char *key) {
  const int fd = get_database_fd();
  struct BTree *cache = get_cache();

  struct KVPair *data = find_kv_from_btree(cache, key);

  if (data && data->type == TELLY_UNSPECIFIED) {
    data->value = malloc(sizeof(value_t));
    lseek(fd, data->pos.start_at, SEEK_SET);
    read(fd, &data->type, 1);

    switch (data->type) {
      case TELLY_NULL: {
        uint8_t c;
        data->value->null = NULL;
        read(fd, &c, 1);
        break;
      }

      case TELLY_INT: {
        uint8_t c;
        data->value->integer = 0;

        while (read(fd, &c, 1) != 0 && c != 0x1E) {
          data->value->integer = (data->value->integer << 8) | c;
        }

        break;
      }

      case TELLY_STR: {
        data->value->string.len = 0;
        data->value->string.value = malloc(33);

        while (
          read(fd, &data->value->string.value[data->value->string.len], 1) != 0
          &&
          data->value->string.value[data->value->string.len] != 0x1E
        ) {
          data->value->string.len += 1;

          if (data->value->string.len % 32 == 0) {
            data->value->string.value = realloc(data->value->string.value, data->value->string.len + 33);
          }
        }

        data->value->string.value = realloc(data->value->string.value, data->value->string.len + 1);
        data->value->string.value[data->value->string.len] = '\0';

        break;
      }

      case TELLY_BOOL: {
        uint8_t c;
        read(fd, &data->value->boolean, 1);
        read(fd, &c, 1);
        break;
      }

      default:
        break;
    }
  }

  return data;
}
