// Includes B-Tree structure and its methods

#pragma once

#include "database.h"

#include <stdint.h>
#include <stdbool.h>

struct BTreeIntegers {
  uint8_t order;
  uint8_t leaf_min;
  uint8_t internal_min;
};

struct BTreeValue {
  uint64_t index;
  void *data;
};

struct BTreeNode {
  struct BTreeValue **data;
  struct BTreeNode **children;
  struct BTreeNode *parent;

  uint32_t at;
  uint32_t size;
};

struct BTree {
  struct BTreeNode *root;
  struct BTreeIntegers integers;
  uint32_t size;
};

struct BTree *create_btree(const uint32_t order);
struct BTreeValue **get_values_from_btree(struct BTree *tree, uint32_t *size);

void move_kv(struct BTreeNode *node, const uint32_t at, const uint32_t to);
uint32_t find_node_of_index(struct BTreeNode **result, struct BTreeNode *search, const uint64_t index);

struct BTreeValue *insert_value_to_btree(struct BTree *tree, const uint64_t index, void *data);
struct BTreeValue *find_value_from_btree(struct BTree *tree, const uint64_t index, void *arg, bool (*check)(void *value, void *arg));
bool delete_value_from_btree(struct BTree *tree, const uint64_t index, void (*free_value)(void *value));

void free_btree(struct BTree *tree, void (*free_value)(void *value));
