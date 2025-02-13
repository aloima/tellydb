#include "../../headers/telly.h"

#include <stdint.h>
#include <stdlib.h>
#include <math.h>

struct BTree *create_btree(const uint32_t order) {
  struct BTree *tree;

  if (posix_memalign((void **) &tree, 16, sizeof(struct BTree)) == 0) {
    const uint32_t k = (order - 1);

    tree->size = 0;
    tree->root = NULL;
    tree->integers = (struct BTreeIntegers) {
      .order = order,
      .leaf_min = ceil((float) k / 2),
      .internal_min = (k / 2)
    };

    return tree;
  } else {
    return NULL;
  }
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
  if (!tree->root) return NULL;

  struct BTreeValue **values;

  if (posix_memalign((void **) &values, 8, (tree->size * sizeof(struct BTreeValue *))) == 0) {
    get_values_from_node(values, size, tree->root);

    if (*size == 0) {
      free(values);
      return NULL;
    } else {
      return values;
    }
  } else {
    return NULL;
  }
}
