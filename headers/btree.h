#pragma once

#include "database.h"

#include <stdint.h>

struct BTreeNode {
  struct KVPair **data;
  uint32_t size;

  struct BTreeNode **leafs;
  struct BTreeNode *top;
  uint32_t leaf_at;
};

struct BTree {
  struct BTreeNode *root;
  uint32_t max;
  uint32_t size;
};

struct BTree *create_btree(const uint32_t max);
struct KVPair **get_kvs_from_btree(struct BTree *tree, uint32_t *size);

void move_kv(struct BTreeNode *node, const uint32_t at, const uint32_t to);
uint32_t find_node_of_kv(struct BTreeNode **result, struct BTreeNode *search, const char *key);

void sort_kvs_by_pos(struct KVPair **kvs, const uint32_t size);

struct KVPair *insert_kv_to_btree(struct BTree *tree, string_t key, value_t *value, enum TellyTypes type, const off_t start_at, const off_t end_at);
struct KVPair *find_kv_from_btree(struct BTree *tree, const char *key);
bool delete_kv_from_btree(struct BTree *tree, const char *key);

void free_btree(struct BTree *tree);

