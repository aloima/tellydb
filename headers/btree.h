#pragma once

#include "database.h"

#include <stdint.h>

struct BTreeIntegers {
  uint8_t order;
  uint8_t leaf_min;
  uint8_t internal_min;
};

struct BTreeNode {
  struct KVPair **data;
  uint32_t size;

  struct BTreeNode **children;
  struct BTreeNode *parent;
  uint32_t at;
};

struct BTree {
  struct BTreeNode *root;
  struct BTreeIntegers integers;
  uint32_t size;
};

struct BTree *create_btree(const uint32_t max);
struct KVPair **get_kvs_from_btree(struct BTree *tree, uint32_t *size);

void move_kv(struct BTreeNode *node, const uint32_t at, const uint32_t to);
uint32_t find_node_of_kv(struct BTreeNode **result, struct BTreeNode *search, const char *key);

void sort_kvs_by_pos(struct KVPair **kvs, const uint32_t size);

struct KVPair *insert_kv_to_btree(struct BTree *tree, string_t key, void *value, enum TellyTypes type, const off_t start_at, const off_t end_at);
struct KVPair *find_kv_from_btree(struct BTree *tree, const char *key);
bool delete_kv_from_btree(struct BTree *tree, const char *key);

void free_btree(struct BTree *tree);

