#include "../../headers/btree.h"

#include <stdint.h>
#include <stdlib.h>
#include <math.h>

struct BTree *create_btree(const uint32_t order) {
  const uint32_t k = (order - 1);
  struct BTree *tree = malloc(sizeof(struct BTree));
  tree->size = 0;
  tree->root = NULL;
  tree->integers = (struct BTreeIntegers) {
    .order = order,
    .leaf_min = ceil((float) k / 2),
    .internal_min = k / 2
  };

  return tree;
}

static void get_values_from_node(struct BTreeValue **values, uint32_t *size, struct BTreeNode *node) {
  if (node->children) {
    for (uint32_t i = 0; i < node->size; ++i) {
      get_values_from_node(values, size, node->children[i]);

      if (node->data[i]->data) {
        values[*size] = node->data[i];
        *size += 1;
      }
    }

    get_values_from_node(values, size, node->children[node->size]);
  } else {
    for (uint32_t i = 0; i < node->size; ++i) {
      values[*size] = node->data[i];
      *size += 1;
    }
  }
}

struct BTreeValue **get_values_from_btree(struct BTree *tree, uint32_t *size) {
  *size = 0;
  if (!tree->root) return NULL;

  struct BTreeValue **values = malloc(tree->size * sizeof(struct BTreeValue *));
  get_values_from_node(values, size, tree->root);

  if (*size == 0) {
    free(values);
    return NULL;
  } else {
    return realloc(values, *size * sizeof(struct BTreeValue *));
  }
}
