#include "../../headers/telly.h"

static struct KVPair *find_kv_from_node(struct BTreeNode *node, const char *key) {
  const char c = key[0];

  if (node->leafs != NULL) {
    for (uint32_t i = 0; i < node->size; ++i) {
      struct KVPair *pair = node->data[i];
      const char *pair_key = pair->key.value;

      if (streq(key, pair_key)) return pair;
      else if (c <= pair_key[0]) return find_kv_from_node(node->leafs[i], key);
    }

    return find_kv_from_node(node->leafs[node->size], key);
  } else {
    for (uint32_t i = 0; i < node->size; ++i) {
      struct KVPair *pair = node->data[i];

      if (streq(pair->key.value, key)) return pair;
    }
  }

  return NULL;
}

struct KVPair *find_kv_from_btree(struct BTree *tree, const char *key) {
  if (tree->root != NULL) return find_kv_from_node(tree->root, key);
  else return NULL;
}
