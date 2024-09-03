#include "../../headers/telly.h"

#include <stdint.h>
#include <string.h>

void set_kv(struct KVPair *pair, char *key, void *value, enum TellyTypes type) {
  pair->type = type;
  set_string(&pair->key, key, -1);

  switch (type) {
    case TELLY_STR:
      set_string(&pair->value.string, value, -1);
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

void move_kv(struct BTreeNode *node, int32_t index) {
  struct KVPair *last_addr = node->data[node->size - 1];
  memcpy(node->data + index + 1, node->data + index, (node->size - index - 1) * sizeof(struct KVPair *));
  node->data[index] = last_addr;
}

uint32_t get_total_size_of_node(struct BTreeNode *node) {
  if (node != NULL) {
    uint32_t res = node->size;

    for (uint32_t i = 0; i < node->leaf_count; ++i) {
      res += get_total_size_of_node(node->leafs[i]);
    }

    return res;
  } else return 0;
}

uint32_t find_index_of_node(struct BTreeNode *node, char *key) {
  const char c = key[0];

  for (uint32_t i = 0; i < node->size; ++i) {
    if (c <= node->data[i]->key.value[0]) return i;
  }

  return node->size;
}
