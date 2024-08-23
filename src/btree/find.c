#include "../../headers/telly.h"

static struct KVPair *find_kv_from_node(struct BTreeNode *node, const char *key) {
  if (node != NULL && node->data != NULL) {
    const char *first = node->data[0]->key.value;
    const char c = key[0];

    if (streq(first, key)) {
      return node->data[0];
    } else if (node->leaf_count != 0 && c <= first[0]) {
      return find_kv_from_node(node->leafs[0], key);
    } else if (node->leaf_count != 0) {
      for (uint32_t i = 0; i < node->size; ++i) {
        struct KVPair *pair = node->data[i];
        const char *pair_key = pair->key.value;

        if (streq(key, pair_key)) {
          return pair;
        } else if (c <= pair_key[0]) {
          return find_kv_from_node(node->leafs[i], key);
        }
      }
    } else {
      for (uint32_t i = 1; i < node->size; ++i) {
        struct KVPair *pair = node->data[i];

        if (streq(pair->key.value, key)) {
          return pair;
        }
      }
    }
  }

  return NULL;
}

struct KVPair *find_kv_from_btree(struct BTree *tree, const char *key) {
  return find_kv_from_node(tree->root, key);
}
