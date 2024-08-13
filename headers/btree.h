#include "database.h"

#include <stdint.h>

#ifndef BTREE_H
  #define BTREE_H

  struct BTreeNode {
    struct KVPair *data;
    uint32_t size;

    struct BTreeNode **leafs;
    uint32_t leaf_count;

    struct BTreeNode *top;
  };

  struct BTree {
    struct BTreeNode *root;
    uint32_t max;
  };

  struct BTree *create_btree(const uint32_t max);
  struct BTreeNode *create_btree_node();

  void add_kv_to_node(struct BTreeNode *node, char *key, void *value, uint32_t type);
  void del_kv_from_node(struct BTreeNode *node, char *key);

  void insert_kv_to_btree(struct BTree *tree, char *key, void *value, uint32_t type);
  struct KVPair find_kv_from_btree(struct BTree *tree, const char *key);

  void free_btree(struct BTree *tree);
#endif
