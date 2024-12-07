#include "../../headers/btree.h"
#include "../../headers/database.h"

#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

static struct BTreeValue *insert_value_to_node(struct BTree *tree, struct BTreeNode *node, struct BTreeValue *value, const uint32_t value_at) {
  tree->size += 1;
  node->size += 1;
  node->data = realloc(node->data, node->size * sizeof(struct BTreeValue *));

  memcpy(node->data + value_at + 1, node->data + value_at, (node->size - value_at - 1) * sizeof(struct BTreeValue *));
  node->data[value_at] = value;

  const uint32_t order = tree->integers.order;

  if (node->size == order) {
    const uint32_t at = (order - 1) / 2;

    if (node->parent) {
      do {
        struct BTreeValue *middle = node->data[at];

        node->parent->size += 1;
        node->parent->data = realloc(node->parent->data, node->parent->size * sizeof(struct BTreeValue *));
        memcpy(node->parent->data + node->at + 1, node->parent->data + node->at, (node->parent->size - 1 - node->at) * sizeof(struct BTreeValue *));
        node->parent->data[node->at] = middle;

        node->parent->children = realloc(node->parent->children, (node->parent->size + 1) * sizeof(struct BTreeNode *));
        node->parent->children[node->parent->size] = malloc(sizeof(struct BTreeNode));

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

        const uint8_t size_n = leaf->size * sizeof(struct BTreeValue *);
        leaf->data = malloc(size_n);
        memcpy(leaf->data, node->data + at + 1, size_n);

        if (node->children) {
          const uint32_t leaf_count = (leaf->size + 1);
          const uint8_t leaf_count_n = leaf_count * sizeof(struct BTreeNode *);
          leaf->children = malloc(leaf_count_n);
          memcpy(leaf->children, node->children + at + 1, leaf_count_n);

          for (uint32_t i = 0; i < leaf_count; ++i) {
            leaf->children[i]->parent = leaf;
            leaf->children[i]->at = i;
          }

          node->size = at;
          node->data = realloc(node->data, node->size * sizeof(struct BTreeValue *));
          node->children = realloc(node->children, (node->size + 1) * sizeof(struct BTreeNode *));
        } else {
          leaf->children = NULL;

          node->size = at;
          node->data = realloc(node->data, node->size * sizeof(struct BTreeValue *));
        }

        node = node->parent;
      } while (node->parent && node->size == order);

      if (node->size == order) {
        const uint32_t leaf_count = node->size + 1;
        struct BTreeNode *children[leaf_count];
        memcpy(children, node->children, leaf_count * sizeof(struct BTreeNode *));

        free(node->children);
        node->children = malloc(2 * sizeof(struct BTreeNode *));
        node->children[0] = malloc(sizeof(struct BTreeNode));
        node->children[1] = malloc(sizeof(struct BTreeNode));

        struct BTreeNode *leaf;
        uint8_t leaf_count_of_leaf, size_n, leaf_count_n;

        leaf = node->children[0];
        leaf->parent = node;
        leaf->at = 0;
        leaf->size = at;
        leaf_count_of_leaf = (leaf->size + 1);

        size_n = leaf->size * sizeof(struct BTreeValue *);
        leaf->data = malloc(size_n);
        memcpy(leaf->data, node->data, size_n);

        leaf_count_n = leaf_count_of_leaf * sizeof(struct BTreeNode *);
        leaf->children = malloc(leaf_count_n);
        memcpy(leaf->children, children, leaf_count_n);

        for (uint32_t i = 0; i < leaf_count_of_leaf; ++i) {
          leaf->children[i]->parent = leaf;
          leaf->children[i]->at = i;
        }

        leaf = node->children[1];
        leaf->parent = node;
        leaf->at = 1;
        leaf->size = node->size - at - 1;
        leaf_count_of_leaf = (leaf->size + 1);

        size_n = leaf->size * sizeof(struct BTreeValue *);
        leaf->data = malloc(size_n);
        memcpy(leaf->data, node->data + at + 1, size_n);

        leaf_count_n = leaf_count_of_leaf * sizeof(struct BTreeNode *);
        leaf->children = malloc(leaf_count_n);
        memcpy(leaf->children, children + at + 1, leaf_count_n);

        for (uint32_t i = 0; i < leaf_count_of_leaf; ++i) {
          leaf->children[i]->parent = leaf;
          leaf->children[i]->at = i;
        }

        node->size = 1;
        node->data[0] = node->data[at];
        node->data = realloc(node->data, node->size * sizeof(struct BTreeValue *));
      }
    } else {
      node->children = malloc(2 * sizeof(struct BTreeNode *));
      node->children[0] = malloc(sizeof(struct BTreeNode));
      node->children[1] = malloc(sizeof(struct BTreeNode));

      struct BTreeNode *leaf = node->children[0];
      leaf->parent = node;
      leaf->at = 0;
      leaf->children = NULL;
      leaf->size = at;

      uint8_t size_n = leaf->size * sizeof(struct BTreeValue *);
      leaf->data = malloc(size_n);
      memcpy(leaf->data, node->data, size_n);

      leaf = node->children[1];
      leaf->parent = node;
      leaf->at = 1;
      leaf->children = NULL;
      leaf->size = node->size - at - 1;

      size_n = leaf->size * sizeof(struct BTreeValue *);
      leaf->data = malloc(size_n);
      memcpy(leaf->data, node->data + at + 1, size_n);

      node->size = 1;
      node->data[0] = node->data[at];
      node->data = realloc(node->data, node->size * sizeof(struct BTreeValue *));
    }
  }

  return value;
}

struct BTreeValue *insert_value_to_btree(struct BTree *tree, uint64_t index, void *data) {
  struct BTreeValue *value = malloc(sizeof(struct BTreeValue));
  value->data = data;
  value->index = index;

  if (!tree->root) {
    tree->root = malloc(sizeof(struct BTreeNode));
    tree->root->children = NULL;
    tree->root->parent = NULL;
    tree->root->at = 0;

    tree->size = 1;
    tree->root->size = 1;
    tree->root->data = malloc(sizeof(struct BTreeValue *));
    tree->root->data[0] = value;

    return value;
  } else {
    struct BTreeNode *node;
    const uint32_t value_at = find_node_of_index(&node, tree->root, index);

    return insert_value_to_node(tree, node, value, value_at);
  }

  return NULL;
}
