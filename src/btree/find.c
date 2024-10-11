#include "../../headers/btree.h"
#include "../../headers/utils.h"

#include <string.h>

static struct KVPair *find_kv_from_node(struct BTreeNode *node, const char *key) {
  if (node->leafs != NULL) {
    for (uint32_t i = 0; i < node->size; ++i) {
      struct KVPair *kv = node->data[i];
      const char *kv_key = kv->key.value;
      const int search = strcmp(key, kv_key);

      if (search == 0) return kv;
      else if (search < 0) return find_kv_from_node(node->leafs[i], key);
    }

    return find_kv_from_node(node->leafs[node->size], key);
  } else {
    for (uint32_t i = 0; i < node->size; ++i) {
      struct KVPair *kv = node->data[i];

      if (streq(kv->key.value, key)) return kv;
    }
  }

  return NULL;
}

struct KVPair *find_kv_from_btree(struct BTree *tree, const char *key) {
  if (tree->root) return find_kv_from_node(tree->root, key);
  else return NULL;
}
