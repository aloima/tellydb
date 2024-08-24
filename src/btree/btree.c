#include "../../headers/telly.h"

#include <stdint.h>
#include <stdlib.h>

struct BTree *create_btree(const uint32_t max) {
  struct BTree *tree = malloc(sizeof(struct BTree));
  tree->max = max;
  tree->root = NULL;

  return tree;
}

static void get_kvs_from_node(struct KVPair **pairs, uint32_t *index, struct BTreeNode *node) {
  for (uint32_t i = 0; i < node->size; ++i) {
    pairs[*index] = node->data[i];
    *index += 1;
  }

  for (uint32_t i = 0; i < node->leaf_count; ++i) {
    get_kvs_from_node(pairs, index, node->leafs[i]);
  }
}

struct KVPair **get_kvs_from_btree(struct BTree *tree) {
  if (tree->root) {
    struct KVPair **pairs = malloc(get_total_size_of_node(tree->root) * sizeof(struct KVPair *));
    uint32_t index = 0;

    get_kvs_from_node(pairs, &index, tree->root);
    return pairs;
  } else return NULL;
}
