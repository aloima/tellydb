#include "../../headers/database.h"
#include "../../headers/btree.h"
#include "../../headers/utils.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <unistd.h>

void get_all_keys() {
  const int fd = get_database_fd();
  struct BTree *cache = get_cache();

  uint8_t first;
  lseek(fd, 10, SEEK_SET);

  while (read(fd, &first, 1)) {
    string_t key;
    uint8_t type;
    off_t start_at, end_at;

    // Key reading
    {
      const uint8_t byte_count = first >> 6;
      key.len = 0;
      read(fd, &key.len, byte_count);

      key.len = (key.len << 6) | (first & 0b111111);
      key.value = malloc(key.len + 1),

      read(fd, key.value, key.len);
      key.value[key.len] = '\0';
    }

    // Value reading
    {
      read(fd, &type, 1);

      switch (type) {
        case TELLY_NULL:
          start_at = lseek(fd, 0, SEEK_CUR);
          end_at = start_at;
          break;

        case TELLY_NUM: {
          uint8_t count;
          read(fd, &count, 1);
          end_at = lseek(fd, count, SEEK_CUR);
          start_at = end_at - count;
          break;
        }

        case TELLY_BOOL:
          end_at = lseek(fd, 1, SEEK_CUR);
          start_at = end_at - 1;
          break;

        case TELLY_STR: {
          uint8_t first;
          read(fd, &first, 1);

          const uint8_t byte_count = first >> 6;
          uint32_t length = 0;
          read(fd, &length, byte_count);
          length = (length << 6) | (first & 0b111111);

          end_at = lseek(fd, length, SEEK_CUR);
          start_at = end_at - length - byte_count - 1;
          break;
        }

        case TELLY_LIST: {
          uint32_t size = 0;
          off_t length = 4;

          read(fd, &size, sizeof(uint32_t));

          while (size--) {
            uint8_t node_type;
            read(fd, &node_type, 1);

            switch (node_type) {
              case TELLY_NULL:
                length += 1;
                break;

              case TELLY_NUM: {
                uint8_t count;
                read(fd, &count, 1);
                lseek(fd, count, SEEK_CUR);
                length += 2 + count;
                break;
              }

              case TELLY_BOOL:
                lseek(fd, 1, SEEK_CUR);
                length += 2;
                break;

              case TELLY_STR: {
                uint32_t string_length = 0;

                uint8_t first;
                read(fd, &first, 1);

                const uint8_t byte_count = first >> 6;
                lseek(fd, byte_count, SEEK_CUR);

                read(fd, &string_length, byte_count);
                string_length = (string_length << 6) | (first & 0b111111);

                lseek(fd, string_length, SEEK_CUR);
                length += 2 + byte_count + string_length;
                break;
              }
            }
          }

          end_at = lseek(fd, 0, SEEK_CUR);
          start_at = end_at - length;
          break;
        }

        default:
          start_at = 0;
          end_at = 0;
      }
    }

    insert_kv_to_btree(cache, key, NULL, type, start_at, end_at);
    free(key.value);
  }
}

struct KVPair *get_data(const char *key) {
  struct KVPair *data = get_kv_from_cache(key);

  if (data && !data->value) {
    const int fd = get_database_fd();

    const off_t start_at = data->pos.start_at;
    const off_t end_at = data->pos.end_at;

    lseek(fd, start_at, SEEK_SET);

    switch (data->type) {
      case TELLY_NUM: {
        const uint8_t count = end_at - start_at;
        data->value = malloc(sizeof(long));
        memset(data->value, 0, sizeof(long)); // to initialize all bytes of long number
        read(fd, data->value, count);

        break;
      }

      case TELLY_STR: {
        string_t *string = (data->value = malloc(sizeof(string_t)));

        uint8_t first;
        read(fd, &first, 1);

        const uint8_t byte_count = first >> 6;
        lseek(fd, byte_count, SEEK_CUR);

        string->len = data->pos.end_at - data->pos.start_at - byte_count - 1;
        string->value = malloc(string->len);
        read(fd, string->value, string->len);

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

          switch (type) {
            case TELLY_NULL:
              node = create_listnode(NULL, TELLY_NULL);
              break;

            case TELLY_NUM: {
              long *number = malloc(sizeof(long));
              memset(number, 0, sizeof(long));

              uint8_t count;
              read(fd, &count, 1);
              read(fd, number, count);

              node = create_listnode(number, TELLY_NUM);
              break;
            }

            case TELLY_STR: {
              string_t *string = malloc(sizeof(string_t));

              uint8_t first;
              read(fd, &first, 1);

              const uint8_t byte_count = first >> 6;
              lseek(fd, byte_count, SEEK_CUR);

              string->len = 0;
              read(fd, &string->len, byte_count);
              string->len = (string->len << 6) | (first & 0b111111);

              string->value = malloc(string->len);
              read(fd, string->value, string->len);

              node = create_listnode(string, TELLY_STR);
              break;
            }

            case TELLY_BOOL: {
              bool *value = malloc(sizeof(bool));
              read(fd, value, 1);

              node = create_listnode(value, TELLY_BOOL);
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
