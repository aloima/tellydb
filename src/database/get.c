#include "../../headers/database.h"
#include "../../headers/btree.h"
#include "../../headers/utils.h"

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

  uint32_t allocated = 32;

  while (read(fd, &c, 1)) {
    if (c != 0x1D) {
      key.value[key.len] = c;
      key.len += 1;

      if (allocated == key.len) {
        key.value = realloc(key.value, allocated + 33);
        allocated += 32;
      }
    } else {
      uint8_t type;
      read(fd, &type, 1);

      const off_t start_at = lseek(fd, 0, SEEK_CUR);
      while (read(fd, &c, 1) == 1 && c != 0x1E);
      const off_t end_at = lseek(fd, 0, SEEK_CUR);

      key.value[key.len] = '\0';
      insert_kv_to_btree(cache, key, NULL, type, start_at, end_at);
      key.len = 0;
    }
  }

  free(key.value);
}

struct KVPair *get_data(const char *key) {
  const int fd = get_database_fd();
  struct BTree *cache = get_cache();

  struct KVPair *data = find_kv_from_btree(cache, key);

  if (data && !data->value) {
    const off_t start_at = data->pos.start_at;
    const off_t end_at = data->pos.end_at;

    data->value = malloc(sizeof(value_t));
    lseek(fd, start_at, SEEK_SET);

    switch (data->type) {
      case TELLY_NULL:
        data->value->null = NULL;
        break;

      case TELLY_NUM: {
        uint8_t count;
        read(fd, &count, 1);

        data->value->number = 0; // to initialize all bytes of long number
        read(fd, &data->value->number, count);

        break;
      }

      case TELLY_STR: {
        data->value->string.len = end_at - start_at - 1;
        data->value->string.value = malloc(data->value->string.len + 1);
        read(fd, data->value->string.value, data->value->string.len);
        data->value->string.value[data->value->string.len] = '\0';

        break;
      }

      case TELLY_BOOL: {
        read(fd, &data->value->boolean, 1);
        break;
      }

      case TELLY_LIST: {
        struct List *list = (data->value->list = create_list());
        read(fd, &list->size, sizeof(uint32_t));

        for (uint32_t i = 0; i < list->size; ++i) {
          struct ListNode *node;
          uint8_t type;
          read(fd, &type, 1);

          uint8_t c;

          switch (type) {
            case TELLY_NULL:
              node = create_listnode(NULL, TELLY_NULL);
              lseek(fd, 1, SEEK_CUR);
              break;

            case TELLY_NUM: {
              long number = 0; // to initialize all bytes of long number

              uint8_t count;
              read(fd, &count, 1);

              read(fd, &number, count);
              lseek(fd, 1, SEEK_CUR); // passing 0x1F

              node = create_listnode(&number, TELLY_NUM);
              break;
            }

            case TELLY_STR: {
              char *value = malloc(33);
              uint64_t len = 0;

              while (read(fd, &c, 1)) {
                if (c != 0x1F) {
                  value[len] = c;
                  len += 1;

                  if (len % 32 == 0) {
                    value = realloc(value, len + 33);
                  }
                } else break;
              }

              value[len] = '\0';
              node = create_listnode(value, TELLY_STR);
              free(value);

              break;
            }

            case TELLY_BOOL:
              read(fd, &c, 1);
              node = create_listnode(&c, TELLY_BOOL);
              lseek(fd, 1, SEEK_CUR);
              break;

            #ifdef __clang__
              #pragma clang diagnostic push
              #pragma clang diagnostic ignored "-Wsometimes-uninitialized"
            #endif

            default:
              break;

            #ifdef __clang__
              #pragma clang diagnostic pop
            #endif
          }

          #ifdef __clang__
            #pragma clang diagnostic push
            #pragma clang diagnostic ignored "-Wunknown-warning-option"
          #endif

          #ifdef __GNUC__
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
          #endif

          node->prev = list->end;
          list->end = node;

          if (i == 0) {
            list->begin = node;
          } else {
            node->prev->next = node;
          }

          #ifdef __GNUC__
            #pragma GCC diagnostic pop
          #endif

          #ifdef __clang__
            #pragma clang diagnostic pop
          #endif
        }
      }

      default:
        break;
    }
  }

  return data;
}
