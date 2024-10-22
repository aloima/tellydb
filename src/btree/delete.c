#include "../../headers/btree.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

// Adds seperator from root to left node, adds elements and children of right node to left node, deletes right node and sets left node as new root
static void merge_and_set_root(struct BTree *tree) {
  struct BTreeNode *root = tree->root;
  struct BTreeNode *right = root->children[1];
  struct BTreeNode *left = root->children[0];

  const uint32_t size = left->size;
  left->size += 1 + right->size; // (separator that moved from parent to left node) + (elements of right node because of merging)

  // Adds separator from root and elements of right node to left node
  left->data = realloc(left->data, left->size * sizeof(struct KVPair *));
  left->data[size] = root->data[0];
  memcpy(left->data + size + 1, right->data, right->size * sizeof(struct KVPair *));

  // Adds children of right node to left node
  if (right->children) {
    const uint32_t child_count = left->size + 1;
    left->children = realloc(left->children, child_count * sizeof(struct BTreeNode *));
    memcpy(left->children + size + 1, right->children, (right->size + 1) * sizeof(struct BTreeNode *));

    for (uint32_t i = size + 1; i < child_count; ++i) {
      left->children[i]->at = i;
    }

    free(right->children);
  }

  // Sets left node as new root
  left->parent = NULL;
  tree->root = left;

  free(right->data);
  free(right);

  free(root->data);
  free(root->children);
  free(root);
}

// TODO: improve readability
// TODO: rebalancing with internal nodes after deleting from leaf
static void rebalance(struct BTree *tree, struct BTreeNode *node, const uint32_t target_at, const uint32_t min) {
  struct BTreeNode *parent = node->parent;
  const uint32_t right_at = node->at + 1;
  const uint32_t left_at = node->at - 1;

  if (node->at != parent->size && parent->children[right_at]->size > min) { // right sibling exists and has more than minimum elements
    struct BTreeNode *right = parent->children[right_at];
    memcpy(node->data + 1, node->data, target_at * sizeof(struct KVPair *));
    node->data[0] = parent->data[left_at];
    parent->data[left_at] = right->data[right->size - 1];
    right->size -= 1;
    memcpy(right->data, right->data + 1, right->size * sizeof(struct KVPair *));
    right->data = realloc(right->data, right->size * sizeof(struct KVPair *));
  } else if (node->at != 0 && parent->children[left_at]->size != min) { // left sibling exists and has more than minimum elements
    struct BTreeNode *left = parent->children[left_at];
    memcpy(node->data + 1, node->data, target_at * sizeof(struct KVPair *));
    node->data[0] = parent->data[left_at];
    parent->data[left_at] = left->data[left->size - 1];
    left->size -= 1;
    left->data = realloc(left->data, left->size * sizeof(struct KVPair *));
  } else if (node->at != 0 && node->at != parent->size && parent->children[left_at]->size == min && parent->children[right_at]->size == min) {
    // left and right siblings exist and have minimum elements
    struct BTreeNode *left = parent->children[left_at];

    node->size -= 1;
    memcpy(node->data + target_at, node->data + target_at + 1, (node->size - target_at) * sizeof(struct KVPair *));

    left->size += node->size + 1;
    left->data = realloc(left->data, left->size * sizeof(struct KVPair *));
    left->data[min] = parent->data[left_at];
    memcpy(left->data + min + 1, node->data, node->size * sizeof(struct KVPair *));

    memcpy(parent->children + node->at, parent->children + right_at, (parent->size - node->at) * sizeof(struct BTreeNode *));
    memcpy(parent->data + left_at, parent->data + node->at, (parent->size - node->at) * sizeof(struct KVPair *));
    parent->children = realloc(parent->children, parent->size * sizeof(struct BTreeNode *));

    for (uint32_t i = node->at; i < parent->size; ++i) {
      parent->children[i]->at -= 1;
    }

    parent->size -= 1;
    parent->data = realloc(parent->data, parent->size * sizeof(struct KVPair *));

    free(node->data);
    free(node);
  } else if (node->at == 0) { // there is no left sibling and right sibling has minimum elements
    struct BTreeNode *right = parent->children[1];

    if (parent == tree->root && parent->size == 1) {
      memcpy(node->data + target_at, node->data + target_at + 1, (node->size - target_at) * sizeof(struct KVPair *));
      merge_and_set_root(tree);
    } else {
      const uint32_t size = node->size;
      node->size += min;
      memcpy(node->data + target_at, node->data + target_at + 1, (size - target_at - 1) * sizeof(struct KVPair *));
      node->data = realloc(node->data, node->size * sizeof(struct KVPair *));
      node->data[min - 1] = parent->data[0];
      memcpy(node->data + min, right->data, min * sizeof(struct KVPair *));

      const uint32_t child_count = parent->size;
      parent->size -= 1;

      memcpy(parent->children + 1, parent->children + 2, parent->size * sizeof(struct BTreeNode *));
      parent->children = realloc(parent->children, child_count * sizeof(struct BTreeNode *));

      for (uint32_t i = 1; i < child_count; ++i) {
        parent->children[i]->at -= 1;
      }

      if (parent->size < tree->integers.internal_min) {
        rebalance(tree, parent, 0, tree->integers.internal_min);
      } else {
        memcpy(parent->data, parent->data + 1, parent->size * sizeof(struct KVPair *));
        parent->data = realloc(parent->data, parent->size * sizeof(struct KVPair *));
      }

      if (right->children) free(right->children);
      free(right->data);
      free(right);
    }
  } else if (node->at == parent->size) { // there is no right sibling and left sibling has minimum elements
    struct BTreeNode *left = parent->children[left_at];

    if (parent == tree->root && parent->size == 1) {
      memcpy(node->data + target_at, node->data + target_at + 1, (node->size - target_at) * sizeof(struct KVPair *));
      merge_and_set_root(tree);
    } else {
      left->size += min;
      memcpy(node->data + target_at, node->data + target_at + 1, (node->size - target_at - 1) * sizeof(struct KVPair *));
      left->data = realloc(left->data, left->size * sizeof(struct KVPair *));
      left->data[min] = parent->data[parent->size - 1];
      memcpy(left->data + min + 1, node->data, (min - 1) * sizeof(struct KVPair *));

      const uint32_t child_count = parent->size;
      parent->size -= 1;
      parent->children = realloc(parent->children, child_count * sizeof(struct BTreeNode *));

      if (parent->size < tree->integers.internal_min) {
        rebalance(tree, parent, parent->size, tree->integers.internal_min);
      } else {
        parent->data = realloc(parent->data, parent->size * sizeof(struct KVPair *));
      }

      if (node->children) free(node->children);
      free(node->data);
      free(node);
    }
  }
}

static void delete_from_root(struct BTree *tree, struct BTreeNode *root, struct KVPair *target, const uint32_t target_at) {
  free_kv(target);

  if (root->size == 1) {
    free(root->data);
    free(root);
    tree->root = NULL;
  } else {
    root->size -= 1;

    if (root->size != target_at) { // If it is not last KVPair *
      struct KVPair **_target_at = root->data + target_at;
      memcpy(_target_at, _target_at + 1, (root->size - target_at) * sizeof(struct KVPair *));
    }

    root->data = realloc(root->data, root->size * sizeof(struct KVPair *));
  }
}

static void delete_from_leaf(struct BTree *tree, struct BTreeNode *node, struct KVPair *target, const uint32_t target_at) {
  free_kv(target);

  if (node->size == tree->integers.leaf_min) {
    rebalance(tree, node, target_at, tree->integers.leaf_min);
  } else {
    node->size -= 1;

    if (node->size != target_at) { // If it is not last KVPair *
      struct KVPair **_target_at = node->data + target_at;
      memcpy(_target_at, _target_at + 1, (node->size - target_at) * sizeof(struct KVPair *));
    }

    node->data = realloc(node->data, node->size * sizeof(struct KVPair *));
  }
}

bool delete_kv_from_btree(struct BTree *tree, const char *key) {
  struct BTreeNode *node;
  const uint32_t target_at = find_node_of_kv(&node, tree->root, key);

  if (target_at >= node->size) return false;

  struct KVPair *target = node->data[target_at];

  if (streq(target->key.value, key)) {
    tree->size -= 1;

    if (!node->children) {
      if (node->parent) {
        delete_from_leaf(tree, node, target, target_at);
      } else {
        delete_from_root(tree, node, target, target_at);
      }
    } else {
      // TODO
    }

    return true;
  }

  return false;
}
