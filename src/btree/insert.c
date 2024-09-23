#include "../../headers/telly.h"

#include <stdint.h>
#include <stdlib.h>

static void add_kv_to_node(struct BTree *tree, struct BTreeNode *node, struct KVPair *kv) {
  tree->size += 1;

  if (node->size == 0) {
    node->size = 1;
    node->data = malloc(sizeof(struct KVPair *));
    node->data[0] = kv;
    return;
  }

  const uint32_t index = find_index_of_kv(node, kv->key.value);

  node->size += 1;
  node->data = realloc(node->data, node->size * sizeof(struct KVPair *));

  move_kv(node, node->size - 1, index);
  node->data[index] = kv;
}

static struct KVPair *insert_kv_to_node(struct BTree *tree, struct BTreeNode *node, const uint32_t leaf_at, struct KVPair *kv) {
  add_kv_to_node(tree, node, kv);

  if (node->size == tree->max) {
    const uint32_t at = (tree->max - 1) / 2;

    if (node->top) {
      do {
        struct KVPair *middle = node->data[at];
        node->top->size += 1;
        node->top->data = realloc(node->top->data, node->top->size * sizeof(struct KVPair *));
        move_kv(node->top, node->top->size - 1, leaf_at);
        node->top->data[leaf_at] = middle;

        node->top->leafs = realloc(node->top->leafs, (node->top->size + 1) * sizeof(struct BTreeNode *));
        node->top->leafs[node->top->size] = malloc(sizeof(struct BTreeNode));

        struct BTreeNode *leaf = node->top->leafs[node->top->size];

        if (node->top->size != leaf_at) {
          memcpy(node->top->leafs + leaf_at + 2, node->top->leafs + leaf_at + 1, (node->top->size - leaf_at - 1) * sizeof(struct BTreeNode *));
          node->top->leafs[leaf_at + 1] = leaf;
        }

        leaf->leafs = NULL;
        leaf->top = node->top;
        leaf->size = node->size - at - 1;
        leaf->data = malloc(leaf->size * sizeof(struct KVPair *));
        memcpy(leaf->data, node->data + at + 1, leaf->size * sizeof(struct KVPair *));

        leaf = node->top->leafs[leaf_at];
        leaf->size = at;
        leaf->data = realloc(leaf->data, leaf->size * sizeof(struct KVPair *));

        node = node->top;
      } while (node->top);

      if (node->size == tree->max) {
        const uint32_t leaf_count = node->size + 1;
        struct BTreeNode *leafs[leaf_count];
        memcpy(leafs, node->leafs, leaf_count * sizeof(struct BTreeNode *));

        free(node->leafs);
        node->leafs = malloc(2 * sizeof(struct BTreeNode *));
        node->leafs[0] = malloc(sizeof(struct BTreeNode));
        node->leafs[1] = malloc(sizeof(struct BTreeNode));

        struct BTreeNode *leaf = node->leafs[0];
        leaf->top = node;
        leaf->size = at;
        leaf->data = malloc(leaf->size * sizeof(struct KVPair *));
        leaf->leafs = malloc((leaf->size + 1) * sizeof(struct BTreeNode *));
        memcpy(leaf->leafs, leafs, (leaf->size + 1) * sizeof(struct BTreeNode *));
        memcpy(leaf->data, node->data, leaf->size * sizeof(struct KVPair *));

        leaf = node->leafs[1];
        leaf->top = node;
        leaf->size = node->size - at - 1;
        leaf->data = malloc(leaf->size * sizeof(struct KVPair *));
        leaf->leafs = malloc((leaf->size + 1) * sizeof(struct BTreeNode *));
        memcpy(leaf->leafs, leafs + at + 1, (leaf->size + 1) * sizeof(struct BTreeNode *));
        memcpy(leaf->data, node->data + at + 1, leaf->size * sizeof(struct KVPair *));

        node->size = 1;
        node->data[0] = node->data[at];
        node->data = realloc(node->data, node->size * sizeof(struct KVPair *));
      }
    } else {
      node->leafs = malloc(2 * sizeof(struct BTreeNode *));
      node->leafs[0] = malloc(sizeof(struct BTreeNode));
      node->leafs[1] = malloc(sizeof(struct BTreeNode));

      struct BTreeNode *leaf = node->leafs[0];
      leaf->top = node;
      leaf->leafs = NULL;
      leaf->size = at;
      leaf->data = malloc(leaf->size * sizeof(struct KVPair *));
      memcpy(leaf->data, node->data, leaf->size * sizeof(struct KVPair *));

      leaf = node->leafs[1];
      leaf->top = node;
      leaf->leafs = NULL;
      leaf->size = node->size - at - 1;
      leaf->data = malloc(leaf->size * sizeof(struct KVPair *));
      memcpy(leaf->data, node->data + at + 1, leaf->size * sizeof(struct KVPair *));

      node->size = 1;
      node->data[0] = node->data[at];
      node->data = realloc(node->data, node->size * sizeof(struct KVPair *));
    }
  }

  return kv;
}

struct KVPair *insert_kv_to_btree(struct BTree *tree, char *key, void *value, enum TellyTypes type) {
  struct KVPair *kv = malloc(sizeof(struct KVPair));
  set_kv(kv, key, value, type);

  if (tree->root == NULL) {
    tree->root = malloc(sizeof(struct BTreeNode));
    tree->root->size = 0;
    tree->root->leafs = NULL;
    tree->root->top = NULL;

    add_kv_to_node(tree, tree->root, kv);
  } else {
    uint32_t leaf_at = 0;
    struct BTreeNode *node = find_node_of_kv(tree->root, &leaf_at, key);

    insert_kv_to_node(tree, node, leaf_at, kv);
  }

  return kv;
}
