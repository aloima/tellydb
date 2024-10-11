#include "../../headers/btree.h"
#include "../../headers/database.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <unistd.h>

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

  memcpy(node->data + index + 1, node->data + index, (node->size - index - 1) * sizeof(struct KVPair *));
  node->data[index] = kv;
}

static struct KVPair *insert_kv_to_node(struct BTree *tree, struct BTreeNode *node, struct KVPair *kv) {
  add_kv_to_node(tree, node, kv);

  if (node->size == tree->max) {
    uint32_t at = (tree->max - 1) / 2;

    if (node->top) {
      do {
        struct KVPair *middle = node->data[at];

        node->top->size += 1;
        node->top->data = realloc(node->top->data, node->top->size * sizeof(struct KVPair *));
        memcpy(node->top->data + node->leaf_at + 1, node->top->data + node->leaf_at, (node->top->size - 1 - node->leaf_at) * sizeof(struct KVPair *));
        node->top->data[node->leaf_at] = middle;

        node->top->leafs = realloc(node->top->leafs, (node->top->size + 1) * sizeof(struct BTreeNode *));
        node->top->leafs[node->top->size] = malloc(sizeof(struct BTreeNode));

        struct BTreeNode *leaf = node->top->leafs[node->top->size];

        if (node->top->size != node->leaf_at) {
          memcpy(node->top->leafs + node->leaf_at + 2, node->top->leafs + node->leaf_at + 1, (node->top->size - node->leaf_at - 1) * sizeof(struct BTreeNode *));
          leaf->leaf_at = node->leaf_at + 1;
          node->top->leafs[node->leaf_at + 1] = leaf;

          const uint32_t leaf_count = node->top->size + 1;

          for (uint32_t i = node->leaf_at + 2; i < leaf_count; ++i) {
            node->top->leafs[i]->leaf_at += 1;
          }
        } else {
          leaf->leaf_at = node->top->size;
        }

        leaf->top = node->top;
        leaf->size = node->size - at - 1;

        const uint8_t size_n = leaf->size * sizeof(struct KVPair *);
        leaf->data = malloc(size_n);
        memcpy(leaf->data, node->data + at + 1, size_n);

        if (node->leafs) {
          const uint32_t leaf_count = (leaf->size + 1);
          const uint8_t leaf_count_n = leaf_count * sizeof(struct BTreeNode *);
          leaf->leafs = malloc(leaf_count_n);
          memcpy(leaf->leafs, node->leafs + at + 1, leaf_count_n);

          for (uint32_t i = 0; i < leaf_count; ++i) {
            leaf->leafs[i]->top = leaf;
            leaf->leafs[i]->leaf_at = i;
          }

          node->size = at;
          node->data = realloc(node->data, node->size * sizeof(struct KVPair *));
          node->leafs = realloc(node->leafs, (node->size + 1) * sizeof(struct BTreeNode *));
        } else {
          leaf->leafs = NULL;

          node->size = at;
          node->data = realloc(node->data, node->size * sizeof(struct KVPair *));
        }

        node = node->top;
      } while (node->top && node->size == tree->max);

      if (node->size == tree->max) {
        const uint32_t leaf_count = node->size + 1;
        struct BTreeNode *leafs[leaf_count];
        memcpy(leafs, node->leafs, leaf_count * sizeof(struct BTreeNode *));

        free(node->leafs);
        node->leafs = malloc(2 * sizeof(struct BTreeNode *));
        node->leafs[0] = malloc(sizeof(struct BTreeNode));
        node->leafs[1] = malloc(sizeof(struct BTreeNode));

        struct BTreeNode *leaf;
        uint8_t leaf_count_of_leaf, size_n, leaf_count_n;

        leaf = node->leafs[0];
        leaf->top = node;
        leaf->leaf_at = 0;
        leaf->size = at;
        leaf_count_of_leaf = (leaf->size + 1);

        size_n = leaf->size * sizeof(struct KVPair *);
        leaf->data = malloc(size_n);
        memcpy(leaf->data, node->data, size_n);

        leaf_count_n = leaf_count_of_leaf * sizeof(struct BTreeNode *);
        leaf->leafs = malloc(leaf_count_n);
        memcpy(leaf->leafs, leafs, leaf_count_n);

        for (uint32_t i = 0; i < leaf_count_of_leaf; ++i) {
          leaf->leafs[i]->top = leaf;
          leaf->leafs[i]->leaf_at = i;
        }

        leaf = node->leafs[1];
        leaf->top = node;
        leaf->leaf_at = 1;
        leaf->size = node->size - at - 1;
        leaf_count_of_leaf = (leaf->size + 1);

        size_n = leaf->size * sizeof(struct KVPair *);
        leaf->data = malloc(size_n);
        memcpy(leaf->data, node->data + at + 1, size_n);

        leaf_count_n = leaf_count_of_leaf * sizeof(struct BTreeNode *);
        leaf->leafs = malloc(leaf_count_n);
        memcpy(leaf->leafs, leafs + at + 1, leaf_count_n);

        for (uint32_t i = 0; i < leaf_count_of_leaf; ++i) {
          leaf->leafs[i]->top = leaf;
          leaf->leafs[i]->leaf_at = i;
        }

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
      leaf->leaf_at = 0;
      leaf->leafs = NULL;
      leaf->size = at;

      uint8_t size_n = leaf->size * sizeof(struct KVPair *);
      leaf->data = malloc(size_n);
      memcpy(leaf->data, node->data, size_n);

      leaf = node->leafs[1];
      leaf->top = node;
      leaf->leaf_at = 1;
      leaf->leafs = NULL;
      leaf->size = node->size - at - 1;

      size_n = leaf->size * sizeof(struct KVPair *);
      leaf->data = malloc(size_n);
      memcpy(leaf->data, node->data + at + 1, size_n);

      node->size = 1;
      node->data[0] = node->data[at];
      node->data = realloc(node->data, node->size * sizeof(struct KVPair *));
    }
  }

  return kv;
}

struct KVPair *insert_kv_to_btree(struct BTree *tree, string_t key, value_t *value, const enum TellyTypes type, const off_t start_at, const off_t end_at) {
  struct KVPair *kv = malloc(sizeof(struct KVPair));
  set_kv(kv, key, value, type, start_at, end_at);

  if (!tree->root) {
    tree->root = malloc(sizeof(struct BTreeNode));
    tree->root->size = 0;
    tree->root->leafs = NULL;
    tree->root->top = NULL;
    tree->root->leaf_at = 0;

    add_kv_to_node(tree, tree->root, kv);
  } else {
    struct BTreeNode *node = find_node_of_kv(tree->root, key.value);
    insert_kv_to_node(tree, node, kv);
  }

  return kv;
}
