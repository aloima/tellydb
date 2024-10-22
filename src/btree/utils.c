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

uint32_t find_node_of_kv(struct BTreeNode **result, struct BTreeNode *search, const char *key) {
  if (search->children) {
    for (uint32_t i = 0; i < search->size; ++i) {
      const struct KVPair *kv = search->data[i];
      const int res = strcmp(key, kv->key.value);

      if (res < 0) {
        return find_node_of_kv(result, search->children[i], key);
      } else if (res == 0) {
        *result = search;
        return i;
      }
    }

    return find_node_of_kv(result, search->children[search->size], key);
  }

  *result = search;

  for (uint32_t i = 0; i < search->size; ++i) {
    if (strcmp(key, search->data[i]->key.value) <= 0) return i;
  }

  return search->size;
}
