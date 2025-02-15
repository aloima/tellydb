#include "../../headers/telly.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <unistd.h>

static void collect_bytes(const int fd, char *block, const uint16_t block_size, uint16_t *at, const uint32_t count, void *data) {
  uint32_t remaining = count;

  if ((*at + remaining) >= block_size) {
    const uint16_t available = (block_size - *at);
    memcpy(data + (count - remaining), block + *at, available);
    read(fd, block, block_size);
    remaining -= available;

    while (remaining >= block_size) {
      memcpy(data + (count - remaining), block, block_size);
      read(fd, block, block_size);
      remaining -= block_size;
    }

    memcpy(data + (count - remaining), block, remaining);
    *at = remaining;
  } else {
    memcpy(data, block + *at, remaining);
    *at += remaining;
  }
}

static size_t collect_string(string_t *string, const int fd, char *block, const uint16_t block_size, uint16_t *at) {
  string->len = 0;

  uint8_t first;
  collect_bytes(fd, block, block_size, at, 1, &first);

  const uint8_t byte_count = (first >> 6);
  collect_bytes(fd, block, block_size, at, byte_count, &string->len);

  string->len = ((string->len << 6) | (first & 0b111111));
  string->value = malloc(string->len);
  collect_bytes(fd, block, block_size, at, string->len, string->value);

  return (1 + byte_count + string->len);
}

static size_t collect_number(long *number, const int fd, char *block, const uint16_t block_size, uint16_t *at) {
  *number = 0;

  uint8_t byte_count;
  collect_bytes(fd, block, block_size, at, 1, &byte_count);
  collect_bytes(fd, block, block_size, at, byte_count, number);

  return (1 + byte_count);
}

static size_t collect_kv(struct KVPair *kv, const int fd, char *block, const uint16_t block_size, uint16_t *at) {
  string_t key;
  void *value = NULL;
  uint8_t type;

  size_t collected_bytes = collect_string(&key, fd, block, block_size, at) + 1;
  collect_bytes(fd, block, block_size, at, 1, &type);

  switch (type) {
    case TELLY_NULL:
      break;

    case TELLY_NUM:
      value = malloc(sizeof(long));
      collected_bytes += collect_number(value, fd, block, block_size, at);
      break;

    case TELLY_STR:
      value = malloc(sizeof(string_t));
      collected_bytes += collect_string(value, fd, block, block_size, at);
      break;

    case TELLY_BOOL:
      value = malloc(sizeof(bool));
      collect_bytes(fd, block, block_size, at, 1, value);
      collected_bytes += 1;
      break;

    case TELLY_HASHTABLE: {
      uint32_t size;
      collect_bytes(fd, block, block_size, at, 4, &size);
      collected_bytes += (5 + size); // includes size bytes, type bytes of fvs and last (0x17) byte

      struct HashTable *table = (value = create_hashtable(size));

      while (true) {
        uint8_t byte;
        collect_bytes(fd, block, block_size, at, 1, &byte);

        if (byte == 0x17) {
          break;
        } else {
          void *fv_value = NULL;

          string_t name;
          collected_bytes += collect_string(&name, fd, block, block_size, at);

          switch (byte) {
            case TELLY_NULL:
              break;

            case TELLY_NUM:
              fv_value = malloc(sizeof(long));
              collected_bytes += collect_number(fv_value, fd, block, block_size, at);
              break;

            case TELLY_STR:
              fv_value = malloc(sizeof(string_t));
              collected_bytes += collect_string(fv_value, fd, block, block_size, at);
              break;

            case TELLY_BOOL:
              fv_value = malloc(sizeof(bool));
              collect_bytes(fd, block, block_size, at, 1, fv_value);
              collected_bytes += 1;
              break;
          }

          add_fv_to_hashtable(table, name, fv_value, byte);
          free(name.value);
        }
      }

      break;
    }

    case TELLY_LIST: {
      uint32_t size;
      collect_bytes(fd, block, block_size, at, 4, &size);
      collected_bytes += (4 + size); // includes size bytes and type bytes of listnodes

      struct List *list = (value = create_list());
      list->size = size;

      for (uint32_t i = 0; i < size; ++i) {
        uint8_t byte;
        void *list_value = NULL;
        collect_bytes(fd, block, block_size, at, 1, &byte);

        switch (byte) {
          case TELLY_NULL:
            break;

          case TELLY_NUM:
            list_value = malloc(sizeof(long));
            collected_bytes += collect_number(list_value, fd, block, block_size, at);
            break;

          case TELLY_STR:
            list_value = malloc(sizeof(string_t));
            collected_bytes += collect_string(list_value, fd, block, block_size, at);
            break;

          case TELLY_BOOL:
            list_value = malloc(sizeof(bool));
            collect_bytes(fd, block, block_size, at, 1, list_value);
            collected_bytes += 1;
            break;
        }

        struct ListNode *node = create_listnode(list_value, byte);

        node->prev = list->end;
        list->end = node;

        if (i == 0) {
          list->begin = node;
        } else {
          node->prev->next = node;
        }
      }
    }

    break;
  }

  set_kv(kv, key, value, type);
  free(key.value);

  return collected_bytes;
}

void get_all_data_from_file(const int fd, off64_t file_size, char *block, const uint16_t block_size, const uint16_t filled_block_size) {
  struct BTree *cache = get_cache();
  uint16_t at = filled_block_size;

  if (at != file_size) {
    if (file_size <= block_size) {
      while (at != file_size) {
        struct KVPair *kv = malloc(sizeof(struct KVPair));
        collect_kv(kv, fd, block, block_size, &at);

        const uint64_t index = hash(kv->key.value, kv->key.len);
        insert_value_to_btree(cache, index, kv);
      }
    } else {
      off64_t collected_bytes = at;

      do {
        while (at <= block_size && collected_bytes != file_size) {
          struct KVPair *kv = malloc(sizeof(struct KVPair));
          collected_bytes += collect_kv(kv, fd, block, block_size, &at);

          const uint64_t index = hash(kv->key.value, kv->key.len);
          insert_value_to_btree(cache, index, kv);
        }

        if (collected_bytes != file_size) read(fd, block, block_size);
        else break;
      } while (true);
    }
  }
}

struct KVPair *get_data(const string_t key) {
  return get_kv_from_cache(key);
}
