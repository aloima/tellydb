#include "../../headers/btree.h"

#include <stdint.h>
#include <string.h>

static struct BTreeValue *find_value_from_node(struct BTreeNode *node, const uint64_t index) {
  if (node->children) {
    for (uint32_t i = 0; i < node->size; ++i) {
      struct BTreeValue *value = node->data[i];
      if (value->index < index) return find_value_from_node(node->children[i], index);
      else if (value->index == index) return value;
    }

    return find_value_from_node(node->children[node->size], index);
  } else {
    for (uint32_t i = 0; i < node->size; ++i) {
      struct BTreeValue *value = node->data[i];
      if (value->index == index) return value;
    }
  }

  return NULL;
}

struct BTreeValue *find_value_from_btree(struct BTree *tree, const uint64_t index) {
  if (tree->root) return find_value_from_node(tree->root, index);
  else return NULL;
}
