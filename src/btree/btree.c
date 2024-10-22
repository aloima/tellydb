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

static void get_kvs_from_node(struct KVPair **kvs, uint32_t *index, struct BTreeNode *node) {
  if (node->children) {
    for (uint32_t i = 0; i < node->size; ++i) {
      get_kvs_from_node(kvs, index, node->children[i]);

      if (node->data[i]->value) {
        kvs[*index] = node->data[i];
        *index += 1;
      }
    }

    get_kvs_from_node(kvs, index, node->children[node->size]);
    return;
  }

  for (uint32_t i = 0; i < node->size; ++i) {
    if (node->data[i]->value) {
      kvs[*index] = node->data[i];
      *index += 1;
    }
  }
}

void sort_kvs_by_pos(struct KVPair **kvs, const uint32_t size) {
  if (size <= 1) return;

  const uint32_t bound_parent = size - 1;

  for (uint32_t i = 0; i < bound_parent; ++i) {
    const uint32_t bound = size - 1 - i;

    for (uint32_t j = 0; j < bound; ++j) {
      if (kvs[j]->pos.start_at <= kvs[j + 1]->pos.start_at) continue;

      struct KVPair *kv = kvs[j + 1];
      kvs[j + 1] = kvs[j];
      kvs[j] = kv;
    }
  }
}

struct KVPair **get_kvs_from_btree(struct BTree *tree, uint32_t *size) {
  *size = 0;
  if (!tree->root) return NULL;

  struct KVPair **kvs = malloc(tree->size * sizeof(struct KVPair *));
  get_kvs_from_node(kvs, size, tree->root);

  if (*size == 0) {
    free(kvs);
    return NULL;
  } else {
    return realloc(kvs, *size * sizeof(struct KVPair *));
  }
}
