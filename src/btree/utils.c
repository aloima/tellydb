#include "../../headers/telly.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

void move_kv(struct BTreeNode *node, uint32_t at, uint32_t to) {
  struct KVPair *pair = node->data[at];

  if (to > at) {
    memcpy(node->data + at, node->data + at + 1, (to - at) * sizeof(struct KVPair *));
  } else if (at > to) {
    memcpy(node->data + to + 1, node->data + to, (at - to) * sizeof(struct KVPair *));
  }

  node->data[to] = pair;
}

uint32_t find_index_of_kv(struct BTreeNode *node, char *key) {
  const char c = key[0];

  for (uint32_t i = 0; i < node->size; ++i) {
    if (c <= node->data[i]->key.value[0]) return i;
  }

  return node->size;
}
