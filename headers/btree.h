#include "database.h"

#include <stdint.h>

#ifndef BTREE_H
  #define BTREE_H

  struct BTreeNode {
    struct KVPair **data;
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

  void set_kv(struct KVPair *pair, char *key, void *value, uint32_t type);
  void *get_kv_val(struct KVPair *pair, uint32_t type);
  void move_kv(struct BTreeNode *node, int32_t index);

  struct KVPair *add_kv_to_node(struct BTreeNode *node, char *key, void *value, uint32_t type);
  struct KVPair *insert_kv_to_btree(struct BTree *tree, char *key, void *value, uint32_t type);

  struct KVPair *find_kv_from_btree(struct BTree *tree, const char *key);

  void del_kv_from_node(struct BTreeNode *node, char *key);

  void free_btree(struct BTree *tree);
#endif
