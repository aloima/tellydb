#include "../../headers/database.h"
#include "../../headers/btree.h"

#include <stdint.h>
#include <stdlib.h>

struct BTree *create_btree(const uint32_t max) {
  struct BTree *tree = malloc(sizeof(struct BTree));
  tree->size = 0;
  tree->max = max;
  tree->root = NULL;

  return tree;
}

static void get_sorted_kvs_from_node(struct KVPair **kvs, uint32_t *index, struct BTreeNode *node) {
  if (node->leafs) {
    for (uint32_t i = 0; i < node->size; ++i) {
      get_sorted_kvs_from_node(kvs, index, node->leafs[i]);
      kvs[*index] = node->data[i];
      *index += 1;
    }

    get_sorted_kvs_from_node(kvs, index, node->leafs[node->size]);
    return;
  }

  for (uint32_t i = 0; i < node->size; ++i) {
    kvs[*index] = node->data[i];
    *index += 1;
  }
}

void sort_kvs_by_pos(struct KVPair **kvs, const uint32_t size) {
  if (size <= 1) return;

  const uint32_t bound_top = size - 1;

  for (uint32_t i = 0; i < bound_top; ++i) {
    const uint32_t bound = size - 1 - i;

    for (uint32_t j = 0; j < bound; ++j) {
      if (kvs[j]->pos.start_at <= kvs[j + 1]->pos.start_at) continue;

      struct KVPair *kv = kvs[j + 1];
      kvs[j + 1] = kvs[j];
      kvs[j] = kv;
    }
  }
}

struct KVPair **get_sorted_kvs_from_btree(struct BTree *tree) {
  if (!tree->root) return NULL;

  struct KVPair **kv = malloc(tree->size * sizeof(struct KVPair *));
  uint32_t index = 0;

  get_sorted_kvs_from_node(kv, &index, tree->root);
  return kv;
}
