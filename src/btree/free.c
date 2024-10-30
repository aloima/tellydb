#include "../../headers/btree.h"

#include <stdlib.h>

static void free_btree_node(struct BTreeNode *node, void (*free_value)(void *value)) {
  if (node->children != NULL) {
    for (uint32_t i = 0; i < node->size; ++i) {
      free_value(node->data[i]->data);
      free(node->data[i]);
      free_btree_node(node->children[i], free_value);
    }

    free_btree_node(node->children[node->size], free_value);
    free(node->children);
  } else {
    for (uint32_t i = 0; i < node->size; ++i) {
      free_value(node->data[i]->data);
      free(node->data[i]);
    }
  }

  free(node->data);
  free(node);
}

void free_btree(struct BTree *tree, void (*free_value)(void *value)) {
  if (tree->root != NULL) free_btree_node(tree->root, free_value);
  free(tree);
}
