#include "../../headers/btree.h"
#include "../../headers/database.h"
#include "../../headers/utils.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

static bool secure_memalign(void **memptr, size_t alignment, size_t size) {
  if (posix_memalign(memptr, alignment, size) == 0) return true;

  write_log(LOG_ERR, "Cannot insert data, out of memory.");
  return false;
}

#define _memalign(memptr, alignment, size) if (!secure_memalign((void **) (memptr), (alignment), (size))) return NULL

static struct BTreeValue *insert_value_to_node(struct BTree *tree, struct BTreeNode *node, struct BTreeValue *value, const uint32_t value_at) {
  const uint8_t order = tree->integers.order;

  tree->size += 1;
  node->size += 1;

  memcpy(node->data + value_at + 1, node->data + value_at, (node->size - value_at - 1) * sizeof(struct BTreeValue *));
  node->data[value_at] = value;

  if (node->size == order) {
    const uint32_t at = (order - 1) / 2;
    const size_t data_size = (order * sizeof(struct BTreeValue *));
    const size_t children_size = ((order + 1) * sizeof(struct BTreeValue *));

    if (node->parent) {
      do {
        struct BTreeValue *middle = node->data[at];

        node->parent->size += 1;
        memcpy(node->parent->data + node->at + 1, node->parent->data + node->at, (node->parent->size - 1 - node->at) * sizeof(struct BTreeValue *));
        node->parent->data[node->at] = middle;

        _memalign(&node->parent->children[node->parent->size], 32, sizeof(struct BTreeNode));

        struct BTreeNode *leaf = node->parent->children[node->parent->size];

        if (node->parent->size != node->at) {
          memcpy(node->parent->children + node->at + 2, node->parent->children + node->at + 1, (node->parent->size - node->at - 1) * sizeof(struct BTreeNode *));
          leaf->at = node->at + 1;
          node->parent->children[node->at + 1] = leaf;

          const uint32_t leaf_count = node->parent->size + 1;

          for (uint32_t i = node->at + 2; i < leaf_count; ++i) {
            node->parent->children[i]->at += 1;
          }
        } else {
          leaf->at = node->parent->size;
        }

        leaf->parent = node->parent;
        leaf->size = node->size - at - 1;

        _memalign(&leaf->data, 8, data_size);

        const uint8_t size_n = (leaf->size * sizeof(struct BTreeValue *));
        memcpy(leaf->data, node->data + at + 1, size_n);

        if (node->children) {
          const uint32_t leaf_count = (leaf->size + 1);

          _memalign(&leaf->children, 8, children_size);
          memcpy(leaf->children, node->children + at + 1, (leaf_count * sizeof(struct BTreeNode *)));

          for (uint32_t i = 0; i < leaf_count; ++i) {
            leaf->children[i]->parent = leaf;
            leaf->children[i]->at = i;
          }

          node->size = at;
        } else {
          node->size = at;
          leaf->children = NULL;
        }

        node = node->parent;
      } while (node->parent && node->size == order);

      if (node->size == order) {
        const uint32_t leaf_count = node->size + 1;
        struct BTreeNode *children[leaf_count];
        memcpy(children, node->children, (leaf_count * sizeof(struct BTreeNode *)));

        _memalign(&node->children[0], 32, sizeof(struct BTreeNode));
        _memalign(&node->children[1], 32, sizeof(struct BTreeNode));

        struct BTreeNode *leaf;
        uint8_t leaf_count_of_leaf;

        leaf = node->children[0];
        leaf->parent = node;
        leaf->at = 0;
        leaf->size = at;
        leaf_count_of_leaf = (leaf->size + 1);

        _memalign(&leaf->data, 8, data_size);
        memcpy(leaf->data, node->data, (leaf->size * sizeof(struct BTreeValue *)));

        _memalign(&leaf->children, 8, children_size);
        memcpy(leaf->children, children, (leaf_count_of_leaf * sizeof(struct BTreeNode *)));

        for (uint32_t i = 0; i < leaf_count_of_leaf; ++i) {
          leaf->children[i]->parent = leaf;
          leaf->children[i]->at = i;
        }

        leaf = node->children[1];
        leaf->parent = node;
        leaf->at = 1;
        leaf->size = node->size - at - 1;
        leaf_count_of_leaf = (leaf->size + 1);

        _memalign(&leaf->data, 8, data_size);
        memcpy(leaf->data, node->data + at + 1, (leaf->size * sizeof(struct BTreeValue *)));

        _memalign(&leaf->children, 8, children_size);
        memcpy(leaf->children, children + at + 1, (leaf_count_of_leaf * sizeof(struct BTreeNode *)));

        for (uint32_t i = 0; i < leaf_count_of_leaf; ++i) {
          leaf->children[i]->parent = leaf;
          leaf->children[i]->at = i;
        }

        node->size = 1;
        node->data[0] = node->data[at];
      }
    } else {
      _memalign(&node->children, 8, children_size);
      _memalign(&node->children[0], 32, sizeof(struct BTreeNode));
      _memalign(&node->children[1], 32, sizeof(struct BTreeNode));

      struct BTreeNode *leaf = node->children[0];
      leaf->parent = node;
      leaf->at = 0;
      leaf->children = NULL;
      leaf->size = at;

      _memalign(&leaf->data, 8, data_size);
      memcpy(leaf->data, node->data, (leaf->size * sizeof(struct BTreeValue *)));

      leaf = node->children[1];
      leaf->parent = node;
      leaf->at = 1;
      leaf->children = NULL;
      leaf->size = node->size - at - 1;

      _memalign(&leaf->data, 8, data_size);
      memcpy(leaf->data, node->data + at + 1, (leaf->size * sizeof(struct BTreeValue *)));

      node->size = 1;
      node->data[0] = node->data[at];
    }
  }

  return value;
}

struct BTreeValue *insert_value_to_btree(struct BTree *tree, uint64_t index, void *data) {
  struct BTreeValue *value;
  _memalign(&value, 16, sizeof(struct BTreeValue));

  value->data = data;
  value->index = index;

  if (!tree->root) {
    _memalign(&tree->root, 32, sizeof(struct BTreeNode));
    tree->root->children = NULL;
    tree->root->parent = NULL;
    tree->root->at = 0;
    tree->size = 0;
    tree->root->size = 0;

    _memalign(&tree->root->data, 8, (sizeof(struct BTreeValue *) * tree->integers.order));
    tree->size = 1;
    tree->root->size = 1;
    tree->root->data[0] = value;

    return value;
  } else {
    struct BTreeNode *node;
    const uint32_t value_at = find_node_of_index(&node, tree->root, index);

    return insert_value_to_node(tree, node, value, value_at);
  }

  return NULL;
}
