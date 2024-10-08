#include "../../headers/btree.h"

#include <stdint.h>
#include <string.h>

void move_kv(struct BTreeNode *node, const uint32_t at, const uint32_t to) {
  struct KVPair *kv = node->data[at];

  if (to > at) {
    memcpy(node->data + at, node->data + at + 1, (to - at) * sizeof(struct KVPair *));
    node->data[to] = kv;
  } else if (at > to) {
    memcpy(node->data + to + 1, node->data + to, (at - to) * sizeof(struct KVPair *));
    node->data[to] = kv;
  }
}

uint32_t find_index_of_kv(struct BTreeNode *node, const char *key) {
  for (uint32_t i = 0; i < node->size; ++i) {
    if (strcmp(key, node->data[i]->key->value) <= 0) return i;
  }

  return node->size;
}

struct BTreeNode *find_node_of_kv(struct BTreeNode *node, const char *key) {
  if (node->leafs) {
    for (uint32_t i = 0; i < node->size; ++i) {
      struct KVPair *kv = node->data[i];

      if (strcmp(key, kv->key->value) <= 0) {
        return find_node_of_kv(node->leafs[i], key);
      }
    }

    return find_node_of_kv(node->leafs[node->size], key);
  }

  return node;
}
