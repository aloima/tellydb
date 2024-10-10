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
    const off_t start_at = data->pos.start_at;
    const off_t end_at = data->pos.end_at;

    data->value = malloc(sizeof(value_t));
    lseek(fd, start_at, SEEK_SET);
    read(fd, &data->type, 1);

    switch (data->type) {
      case TELLY_NULL: {
        data->value->null = NULL;
        break;
      }

      case TELLY_INT: {
        const uint32_t size = end_at - start_at - 3;
        uint8_t c;
        read(fd, &c, 1);
        data->value->integer = c;

        for (uint32_t i = 1; i < size; ++i) {
          read(fd, &c, 1);
          data->value->integer = (data->value->integer << 8) | c;
        }

        break;
      }

      case TELLY_STR: {
        data->value->string.len = end_at - start_at - 2;
        data->value->string.value = malloc(data->value->string.len + 1);
        read(fd, data->value->string.value, data->value->string.len);
        data->value->string.value[data->value->string.len] = '\0';

        break;
      }

      case TELLY_BOOL: {
        read(fd, &data->value->boolean, 1);
        break;
      }

      default:
        break;
    }
  }

  return data;
}
