#include "../../headers/telly.h"

#include <stdlib.h>

static void free_btree_node(struct BTreeNode *node) {
  if (node->leafs != NULL) {
    for (uint32_t i = 0; i < node->size; ++i) {
      struct KVPair *pair = node->data[i];
      free(pair->key.value);

      if (pair->type == TELLY_STR) {
        free(pair->value.string.value);
      }

      free(pair);
      free_btree_node(node->leafs[i]);
    }

    free_btree_node(node->leafs[node->size]);
    free(node->leafs);
  } else {
    for (uint32_t i = 0; i < node->size; ++i) {
      struct KVPair *pair = node->data[i];
      free(pair->key.value);

      if (pair->type == TELLY_STR) {
        free(pair->value.string.value);
      }

      free(pair);
    }
  }

  free(node->data);
  free(node);
}

void free_btree(struct BTree *tree) {
  if (tree->root != NULL) free_btree_node(tree->root);
  free(tree);
}
