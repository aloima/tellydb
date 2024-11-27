#include "../../headers/btree.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

// Adds separator from root to left node, adds elements and children of right node to left node, deletes right node and sets left node as new root
static void merge_and_set_root(struct BTree *tree) {
  struct BTreeNode *root = tree->root;
  struct BTreeNode *right = root->children[1];
  struct BTreeNode *left = root->children[0];

  const uint32_t size = left->size;
  left->size += 1 + right->size; // (separator that moved from parent to left node) + (elements of right node because of merging)

  // Adds separator from root and elements of right node to left node
  left->data = realloc(left->data, left->size * sizeof(struct BTreeValue *));
  left->data[size] = root->data[0];
  memcpy(left->data + size + 1, right->data, right->size * sizeof(struct BTreeValue *));

  // Adds children of right node to left node
  if (right->children) {
    const uint32_t child_count = left->size + 1;
    left->children = realloc(left->children, child_count * sizeof(struct BTreeNode *));
    memcpy(left->children + size + 1, right->children, (right->size + 1) * sizeof(struct BTreeNode *));

    for (uint32_t i = size + 1; i < child_count; ++i) {
      left->children[i]->at = i;
      left->children[i]->parent = left;
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
// TODO: fix deleting value from root if it has children nodes
static void rebalance(struct BTree *tree, struct BTreeNode *node, const uint32_t target_at, const uint32_t min) {
  struct BTreeNode *parent = node->parent;
  const uint32_t right_at = node->at + 1;
  const uint32_t left_at = node->at - 1;

  if (node->at != parent->size && parent->children[right_at]->size > min) { // right sibling exists and has more than minimum elements
    struct BTreeNode *right = parent->children[right_at];
    memcpy(node->data + 1, node->data, target_at * sizeof(struct BTreeValue *));
    node->data[0] = parent->data[node->at];
    parent->data[node->at] = right->data[0];

    if (right->children) {
      node->children = realloc(node->children, (node->size + 1) * sizeof(struct BTreeNode *));
      node->children[node->size] = right->children[0];
      memcpy(right->children, right->children + 1, right->size * sizeof(struct BTreeNode *));

      right->children = realloc(right->children, right->size * sizeof(struct BTreeNode *));
    }

    right->size -= 1;
    memcpy(right->data, right->data + 1, right->size * sizeof(struct BTreeValue *));
    right->data = realloc(right->data, right->size * sizeof(struct BTreeValue *));
  } else if (node->at != 0 && parent->children[left_at]->size != min) { // left sibling exists and has more than minimum elements
    struct BTreeNode *left = parent->children[left_at];
    memcpy(node->data + 1, node->data, target_at * sizeof(struct BTreeValue *));
    node->data[0] = parent->data[left_at];
    parent->data[left_at] = left->data[left->size - 1];

    if (left->children) {
      node->children = realloc(node->children, (node->size + 1) * sizeof(struct BTreeNode *));
      memcpy(node->children + 1, node->children, node->size * sizeof(struct BTreeNode *));
      node->children[0] = left->children[left->size];

      left->children = realloc(left->children, left->size * sizeof(struct BTreeNode *));
    }

    left->size -= 1;
    left->data = realloc(left->data, left->size * sizeof(struct BTreeValue *));
  } else if (node->at != 0 && node->at != parent->size && parent->children[left_at]->size == min && parent->children[right_at]->size == min) {
    // left and right siblings exist and have minimum elements
    struct BTreeNode *left = parent->children[left_at];

    if (parent == tree->root && parent->size == 1) {
      memcpy(node->data + target_at, node->data + target_at + 1, (node->size - target_at) * sizeof(struct BTreeValue *));
      merge_and_set_root(tree);
    } else {
      node->size -= 1;
      memcpy(node->data + target_at, node->data + target_at + 1, (node->size - target_at) * sizeof(struct BTreeValue *));

      left->size += node->size + 1;
      left->data = realloc(left->data, left->size * sizeof(struct BTreeValue *));
      left->data[min] = parent->data[left_at];
      memcpy(left->data + min + 1, node->data, node->size * sizeof(struct BTreeValue *));

      const uint32_t child_count = parent->size;
      parent->size -= 1;

      memcpy(parent->children + node->at, parent->children + right_at, (child_count - node->at) * sizeof(struct BTreeNode *));
      parent->children = realloc(parent->children, child_count * sizeof(struct BTreeNode *));

      for (uint32_t i = node->at; i < child_count; ++i) {
        parent->children[i]->at -= 1;
      }

      if (parent->size < tree->integers.internal_min) {
        rebalance(tree, parent, 0, tree->integers.internal_min);
      } else {
        memcpy(parent->data + left_at, parent->data + node->at, (parent->size - node->at + 1) * sizeof(struct BTreeValue *));
        parent->data = realloc(parent->data, parent->size * sizeof(struct BTreeValue *));
      }

      free(node->data);
      free(node);
    }
  } else if (node->at == 0) { // there is no left sibling and right sibling has minimum elements
    struct BTreeNode *right = parent->children[1];

    if (parent == tree->root && parent->size == 1) {
      // TODO: fix this line
      memcpy(node->data + target_at, node->data + target_at + 1, (node->size - target_at) * sizeof(struct BTreeValue *));
      merge_and_set_root(tree);
    } else {
      const uint32_t size = node->size;
      node->size += min;
      memcpy(node->data + target_at, node->data + target_at + 1, (size - target_at - 1) * sizeof(struct BTreeValue *));

      node->data = realloc(node->data, node->size * sizeof(struct BTreeValue *));
      node->data[min - 1] = parent->data[0];
      memcpy(node->data + min, right->data, min * sizeof(struct BTreeValue *));

      const uint32_t child_count = parent->size;
      memcpy(parent->children + 1, parent->children + 2, (child_count - 1) * sizeof(struct BTreeNode *));
      parent->children = realloc(parent->children, child_count * sizeof(struct BTreeNode *));

      for (uint32_t i = 1; i < child_count; ++i) {
        parent->children[i]->at -= 1;
      }

      if (parent->size == tree->integers.internal_min) {
        rebalance(tree, parent, 0, tree->integers.internal_min);
      } else {
        parent->size -= 1;
        memcpy(parent->data, parent->data + 1, parent->size * sizeof(struct BTreeValue *));
        parent->data = realloc(parent->data, parent->size * sizeof(struct BTreeValue *));
      }

      if (right->children) {
        node->children = realloc(node->children, (node->size + 1) * sizeof(struct BTreeNode *));
        memcpy(node->children + size, right->children, (right->size + 1) * sizeof(struct BTreeNode *));
        free(right->children);
      }

      free(right->data);
      free(right);
    }
  } else if (node->at == parent->size) { // there is no right sibling and left sibling has minimum elements
    struct BTreeNode *left = parent->children[left_at];

    if (parent == tree->root && parent->size == 1) {
      memcpy(node->data + target_at, node->data + target_at + 1, (node->size - target_at) * sizeof(struct BTreeValue *));
      merge_and_set_root(tree);
    } else {
      left->size += min;
      memcpy(node->data + target_at, node->data + target_at + 1, (node->size - target_at - 1) * sizeof(struct BTreeValue *));
      left->data = realloc(left->data, left->size * sizeof(struct BTreeValue *));
      left->data[min] = parent->data[parent->size - 1];
      memcpy(left->data + min + 1, node->data, (min - 1) * sizeof(struct BTreeValue *));

      const uint32_t child_count = parent->size;
      parent->children = realloc(parent->children, child_count * sizeof(struct BTreeNode *));

      if (parent->size == tree->integers.internal_min) {
        rebalance(tree, parent, parent->size - 1, tree->integers.internal_min);
      } else {
        parent->size -= 1;
        parent->data = realloc(parent->data, parent->size * sizeof(struct BTreeValue *));
      }

      if (node->children) {
        left->children = realloc(left->children, (left->size + 1) * sizeof(struct BTreeNode *));
        memcpy(left->children + left->size, node->children, node->size * sizeof(struct BTreeNode *));
        free(node->children);
      }

      free(node->data);
      free(node);
    }
  }
}

static void delete_from_root(struct BTree *tree, struct BTreeNode *root, const uint32_t target_at) {
  if (root->size == 1) {
    free(root->data);
    free(root);
    tree->root = NULL;
  } else {
    root->size -= 1;

    if (root->size != target_at) { // If it is not last BTreeValue *
      struct BTreeValue **_target_at = root->data + target_at;
      memcpy(_target_at, _target_at + 1, (root->size - target_at) * sizeof(struct BTreeValue *));
    }

    root->data = realloc(root->data, root->size * sizeof(struct BTreeValue *));
  }
}

static void delete_from_internal(struct BTree *tree, struct BTreeNode *node, const uint32_t target_at) {
  struct BTreeNode *leaf = node->children[target_at];
  while (leaf->children) leaf = leaf->children[leaf->size];

  const uint32_t separator_at = (leaf->size - 1);
  struct BTreeValue *separator = leaf->data[separator_at];

  node->data[target_at] = separator;
  const uint8_t min = !leaf->children ? tree->integers.leaf_min : tree->integers.internal_min;

  if (leaf->size == min) rebalance(tree, leaf, separator_at, min);
}

static void delete_from_leaf(struct BTree *tree, struct BTreeNode *node, const uint32_t target_at) {
  if (node->size == tree->integers.leaf_min) {
    rebalance(tree, node, target_at, tree->integers.leaf_min);
  } else {
    node->size -= 1;

    if (node->size != target_at) { // If it is not last BTreeValue *
      struct BTreeValue **_target_at = node->data + target_at;
      memcpy(_target_at, _target_at + 1, (node->size - target_at) * sizeof(struct BTreeValue *));
    }

    node->data = realloc(node->data, node->size * sizeof(struct BTreeValue *));
  }
}

bool delete_value_from_btree(struct BTree *tree, const uint64_t index, void (*free_value)(void *value)) {
  struct BTreeNode *node;
  const uint32_t target_at = find_node_of_index(&node, tree->root, index);

  if (target_at >= node->size) return false;

  struct BTreeValue *target = node->data[target_at];

  if (target->index == index) {
    if (free_value) free_value(target->data);
    free(target);
    tree->size -= 1;

    if (!node->children) {
      if (node->parent) {
        delete_from_leaf(tree, node, target_at);
      } else {
        delete_from_root(tree, node, target_at);
      }
    } else {
      delete_from_internal(tree, node, target_at);
    }

    return true;
  }

  return false;
}
