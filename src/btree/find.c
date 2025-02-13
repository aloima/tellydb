#include "../../headers/telly.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

static struct BTreeValue *find_value_from_node(struct BTreeNode *node, const uint64_t index, void *arg, bool (*check)(void *value, void *arg)) {
  if (node->children) {
    if (check) {
      for (uint32_t i = 0; i < node->size; ++i) {
        struct BTreeValue *value = node->data[i];
        if (index < value->index) return find_value_from_node(node->children[i], index, arg, check);
        else if (index == value->index) {
          if (check(value->data, arg)) return value;
          else {
            struct BTreeValue *left = find_value_from_node(node->children[i], index, arg, check);
            if (left) return left;
          }
        }
      }
    } else {
      for (uint32_t i = 0; i < node->size; ++i) {
        struct BTreeValue *value = node->data[i];
        if (index < value->index) return find_value_from_node(node->children[i], index, arg, check);
        else if (index == value->index) return value;
      }
    }

    return find_value_from_node(node->children[node->size], index, arg, check);
  } else {
    if (check) {
      for (uint32_t i = 0; i < node->size; ++i) {
        struct BTreeValue *value = node->data[i];
        if (index == value->index && check(value->data, arg)) return value;
      }
    } else {
      for (uint32_t i = 0; i < node->size; ++i) {
        struct BTreeValue *value = node->data[i];
        if (index == value->index) return value;
      }
    }
  }

  return NULL;
}

struct BTreeValue *find_value_from_btree(struct BTree *tree, const uint64_t index, void *arg, bool (*check)(void *value, void *arg)) {
  if (tree->root) return find_value_from_node(tree->root, index, arg, check);
  else return NULL;
}
