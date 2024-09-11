#include "../../headers/telly.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

void set_kv(struct KVPair *pair, char *key, void *value, enum TellyTypes type) {
  pair->type = type;
  set_string(&pair->key, key, -1, true);

  switch (type) {
    case TELLY_STR:
      set_string(&pair->value.string, value, -1, true);
      break;

    case TELLY_INT:
      pair->value.integer = *((int *) value);
      break;

    case TELLY_BOOL:
      pair->value.boolean = *((bool *) value);
      break;

    case TELLY_NULL:
      pair->value.null = NULL;
      break;
  }
}

void *get_kv_val(struct KVPair *pair, enum TellyTypes type) {
  switch (type) {
    case TELLY_STR:
      return pair->value.string.value;

    case TELLY_INT:
      return &pair->value.integer;

    case TELLY_BOOL:
      return &pair->value.boolean;

    case TELLY_NULL:
      return NULL;

    default:
      return NULL;
  }
}

void move_kv(struct BTreeNode *node, uint32_t at, uint32_t to) {
  struct KVPair *pair = node->data[at];

  if (to > at) {
    memcpy(node->data + at, node->data + at + 1, (to - at) * sizeof(struct KVPair *));
  } else if (at > to) {
    memcpy(node->data + to + 1, node->data + to, (at - to) * sizeof(struct KVPair *));
  }

  node->data[to] = pair;
}

uint32_t get_total_size_of_node(struct BTreeNode *node) {
  if (node != NULL) {
    uint32_t res = node->size;

    if (node->leafs != NULL) {
      for (uint32_t i = 0; i < node->size; ++i) {
        res += get_total_size_of_node(node->leafs[i]);
      }

      res += get_total_size_of_node(node->leafs[node->size]);
    }

    return res;
  } else return 0;
}

uint32_t find_index_of_kv(struct BTreeNode *node, char *key) {
  const char c = key[0];

  for (uint32_t i = 0; i < node->size; ++i) {
    if (c <= node->data[i]->key.value[0]) return i;
  }

  return node->size;
}
