// B-Tree implementation

#include "database.h"

#include <stdint.h>

#ifndef BTREE_H
  #define BTREE_H

  struct BTreeNode {
    struct KVPair **data;
    uint32_t size;

    struct BTreeNode **leafs;
    struct BTreeNode *top;
  };

  struct BTree {
    struct BTreeNode *root;
    uint32_t max;
    uint32_t size;
  };

  struct BTree *create_btree(const uint32_t max);
  struct KVPair **get_sorted_kvs_from_btree(struct BTree *tree);

  void move_kv(struct BTreeNode *node, uint32_t at, uint32_t to);
  uint32_t find_index_of_kv(struct BTreeNode *node, char *key);
  void sort_kvs_by_pos(struct KVPair **pairs, const uint32_t size);

  struct KVPair *insert_kv_to_btree(struct BTree *tree, char *key, void *value, enum TellyTypes type);

  struct KVPair *find_kv_from_btree(struct BTree *tree, const char *key);

  void del_kv_from_node(struct BTreeNode *node, char *key);

  void free_btree(struct BTree *tree);
#endif
