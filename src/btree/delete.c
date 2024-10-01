#include "../../headers/telly.h"

#include <stdint.h>
#include <stdlib.h>

static void delete_from_leaf(struct BTree *tree, struct BTreeNode *node, struct KVPair *target, const uint32_t target_at) {
  if (node->size == 1 && node->top) {
    const uint32_t prev_at = node->leaf_at - 1;
    const uint32_t next_at = node->leaf_at + 1;
    struct BTreeNode *top = node->top;

    const bool is_last_leaf = node->leaf_at == top->size;

    if (node->leaf_at == 0) {
      struct BTreeNode *next = top->leafs[next_at];

      if (next->size == 1) {
        free_kv(node->data[0]);
        free(node->data);
        free(node);

        next->size += 1;
        next->data = realloc(next->data, next->size * sizeof(struct KVPair *));

        const uint8_t at = (next->size - 1);
        struct KVPair *last = (next->data[at] = top->data[0]);
        memcpy(next->data + 1, next->data, at * sizeof(struct KVPair *));
        next->data[0] = last;

        const uint32_t leaf_count = top->size;
        top->size -= 1;

        if (top->size == 0) {
          next->top = top->top;
          next->leaf_at = top->leaf_at;

          if (next->top) {
            next->top->leafs[top->leaf_at] = next;
          } else {
            tree->root = next;
          }

          free(top->data);
          free(top->leafs);
          free(top);
        } else {
          const uint8_t size_n = top->size * sizeof(struct KVPair *);
          memcpy(top->data, top->data + 1, size_n);
          top->data = realloc(top->data, size_n);

          const uint8_t leaf_count_n = leaf_count * sizeof(struct BTreeNode *);
          memcpy(top->leafs, top->leafs + 1, leaf_count_n);
          top->leafs = realloc(top->leafs, leaf_count_n);

          for (uint32_t i = 0; i < leaf_count; ++i) {
            top->leafs[i]->leaf_at = i;
          }
        }
      } else {
        free_kv(node->data[0]);
        node->data[0] = top->data[0];
        top->data[0] = next->data[0];

        next->size -= 1;
        const uint8_t size_n = next->size * sizeof(struct KVPair *);
        memcpy(next->data, next->data + 1, size_n);
        next->data = realloc(next->data, size_n);
      }
    } else if (is_last_leaf) {
      struct BTreeNode *prev = top->leafs[prev_at];

      if (prev->size == 1) {
        free_kv(node->data[0]);
        free(node->data);
        free(node);

        prev->size += 1;
        prev->data = realloc(prev->data, prev->size * sizeof(struct KVPair *));
        prev->data[1] = top->data[prev_at];

        const uint32_t leaf_count = top->size;
        top->size -= 1;

        if (top->size == 0) {
          prev->top = top->top;
          prev->leaf_at = top->leaf_at;

          if (prev->top) {
            prev->top->leafs[prev->leaf_at] = prev;
          } else {
            tree->root = prev;
          }

          free(top->data);
          free(top->leafs);
          free(top);
        } else {
          top->data = realloc(top->data, top->size * sizeof(struct KVPair *));
          top->leafs = realloc(top->leafs, leaf_count * sizeof(struct BTreeNode *));
        }
      } else {
        free_kv(node->data[0]);
        node->data[0] = top->data[prev_at];
        top->data[0] = prev->data[prev->size - 1];

        prev->size -= 1;
        prev->data = realloc(prev->data, prev->size * sizeof(struct KVPair *));
      }
    } else {
      struct BTreeNode *prev = top->leafs[prev_at];
      struct BTreeNode *next = top->leafs[next_at];

      if (prev->size > next->size) {
        free_kv(node->data[0]);
        node->data[0] = top->data[prev_at];

        top->data[node->leaf_at] = prev->data[prev->size - 1];
        prev->size -= 1;
        prev->data = realloc(prev->data, prev->size * sizeof(struct KVPair *));
      } else {
        free_kv(node->data[0]);
        node->data[0] = top->data[node->leaf_at];

        top->data[node->leaf_at] = next->data[0];
        next->size -= 1;

        const uint8_t size_n = next->size * sizeof(struct KVPair *);
        memcpy(next->data, next->data + 1, size_n);
        next->data = realloc(next->data, size_n);
      }
    }
  } else {
    free_kv(target);

    if (node->size == 1) {
      free(node->data);
      free(node);
      tree->root = NULL;
    } else {
      move_kv(node, target_at, node->size - 1);
      node->size -= 1;
      node->data = realloc(node->data, node->size * sizeof(struct KVPair *));
    }
  }
}

bool delete_kv_from_btree(struct BTree *tree, const char *key) {
  struct BTreeNode *node = find_node_of_kv(tree->root, key);

  const uint32_t target_at = find_index_of_kv(node, key);
  if (target_at >= node->size) return false;

  struct KVPair *target = node->data[target_at];

  if (streq(target->key.value, key)) {
    tree->size -= 1;

    if (!node->leafs) {
      delete_from_leaf(tree, node, target, target_at);
    } else {
      // TODO
    }

    return true;
  }

  return false;
}
