#include "../../headers/telly.h"

#include <stdlib.h>

static void free_btree_node(struct BTreeNode *node) {
  for (uint32_t i = 0; i < node->size; ++i) {
    struct KVPair *pair = node->data[i];
    free(pair->key.value);

    if (pair->type == TELLY_STR) {
      free(pair->value.string.value);
    }

    free(pair);
  }

  for (uint32_t i = 0; i < node->leaf_count; ++i) {
    free_btree_node(node->leafs[i]);
  }

  if (node->leaf_count != 0) free(node->leafs);
  free(node->data);
  free(node);
}

void free_btree(struct BTree *tree) {
  free_btree_node(tree->root);
  free(tree);
}
