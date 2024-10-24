#include "../../headers/database.h"
#include "../../headers/btree.h"
#include "../../headers/utils.h"

#include <stdbool.h>
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
        allocated += 32;
        key.value = realloc(key.value, allocated + 33);
      }
    } else {
      uint8_t type;
      read(fd, &type, 1);

      const off_t start_at = lseek(fd, 0, SEEK_CUR);
      off_t end_at;

      switch (type) {
        case TELLY_NULL:
          end_at = lseek(fd, 1, SEEK_CUR);
          break;

        case TELLY_NUM: {
          uint8_t count;
          read(fd, &count, 1);
          end_at = lseek(fd, count + 1, SEEK_CUR);
          break;
        }

        case TELLY_BOOL:
          end_at = lseek(fd, 2, SEEK_CUR);
          break;

        default:
          while (read(fd, &c, 1) == 1 && c != 0x1E);
          end_at = lseek(fd, 0, SEEK_CUR);
      }

      key.value[key.len] = '\0';
      insert_kv_to_btree(cache, key, NULL, type, start_at, end_at);
      key.len = 0;
    }
  }

  free(key.value);
}

// Allocate edilmemiş
struct KVPair *get_data(const char *key) {
  struct KVPair *data = get_kv_from_cache(key);

  if (data && !data->value) {
    const int fd = get_database_fd();

    const off_t start_at = data->pos.start_at;
    const off_t end_at = data->pos.end_at;

    lseek(fd, start_at, SEEK_SET);

    switch (data->type) {
      case TELLY_NUM: {
        uint8_t count;
        read(fd, &count, 1);

        data->value = malloc(sizeof(long));
        memset(data->value, 0, sizeof(long)); // to initialize all bytes of long number
        read(fd, data->value, count);

        break;
      }

      case TELLY_STR: {
        string_t *string = (data->value = malloc(sizeof(string_t)));
        string->len = end_at - start_at - 1;
        string->value = malloc(string->len + 1);
        read(fd, string->value, string->len);
        string->value[string->len] = '\0';

        break;
      }

      case TELLY_BOOL:
        data->value = malloc(sizeof(bool));
        read(fd, data->value, 1);
        break;

      case TELLY_LIST: {
        struct List *list = (data->value = create_list());
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
              long *number = malloc(sizeof(long));
              memset(number, 0, sizeof(long)); // to initialize all bytes of long number

              uint8_t count;
              read(fd, &count, 1);

              read(fd, number, count);
              lseek(fd, 1, SEEK_CUR); // passing 0x1F

              node = create_listnode(number, TELLY_NUM);
              break;
            }

            case TELLY_STR: {
              string_t *value = (value = malloc(sizeof(string_t)));
              value->value = malloc(33);
              value->len = 0;

              while (read(fd, &c, 1)) {
                if (c != 0x1F) {
                  value->value[value->len] = c;
                  value->len += 1;

                  if (value->len % 32 == 0) {
                    value = realloc(value, value->len + 33);
                  }
                } else break;
              }

              value->value[value->len] = '\0';
              node = create_listnode(value, TELLY_STR);

              break;
            }

            case TELLY_BOOL: {
              bool *value = malloc(sizeof(bool));
              read(fd, value, 1);

              node = create_listnode(value, TELLY_BOOL);
              lseek(fd, 1, SEEK_CUR);
              break;
            }

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
