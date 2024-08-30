#include "../../headers/telly.h"

#include <stdint.h>
#include <stdlib.h>

struct KVPair *add_kv_to_node(struct BTreeNode *node, char *key, void *value, uint32_t type) {
  node->size += 1;

  if (node->size == 1) {
    node->data = malloc(sizeof(struct KVPair *));
    node->data[0] = calloc(1, sizeof(struct KVPair));
    set_kv(node->data[0], key, value, type);
    return node->data[0];
  } else {
    node->data = realloc(node->data, node->size * sizeof(struct KVPair *));
    node->data[node->size - 1] = calloc(1, sizeof(struct KVPair));

    const char c = key[0];

    if (node->data[node->size - 2]->key.value[0] <= c) {
      const uint32_t at = node->size - 1;
      set_kv(node->data[at], key, value, type);

      return node->data[at];
    } else if (c <= node->data[0]->key.value[0]) {
      move_kv(node, 0);
      set_kv(node->data[0], key, value, type);

      return node->data[0];
    } else {
      const uint32_t bound = node->size - 2;

      for (uint32_t i = 0; i < bound; ++i) {
        const uint32_t next = i + 1;

        if (node->data[i]->key.value[0] <= c && c <= node->data[next]->key.value[0]) {
          move_kv(node, next);
          set_kv(node->data[next], key, value, type);

          return node->data[next];
        }
      }
    }
  }

  return NULL;
}

struct KVPair *insert_kv_to_btree(struct BTree *tree, char *key, void *value, uint32_t type) {
  if (tree->root == NULL) {
    tree->root = calloc(1, sizeof(struct BTreeNode));
    return add_kv_to_node(tree->root, key, value, type);
  } else {
    struct BTreeNode *node = tree->root;
    char c = key[0];
    int32_t leaf_at = -1;

    if (node->leaf_count != 0) {
      for (uint32_t i = 0; i < node->size; ++i) {
        if (c <= node->data[i]->key.value[0]) {
          leaf_at = i;
          node = node->leafs[i];
          break;
        }
      }

      if (node->top == NULL) {
        leaf_at = node->leaf_count - 1;
        node = node->leafs[leaf_at];
      }
    }

    struct KVPair *res = add_kv_to_node(node, key, value, type);

    if (node->size == tree->max) {
      if (node->leaf_count == 0) {
        if (!node->top) {
          node->leaf_count = 2;
          node->leafs = malloc(node->leaf_count * sizeof(struct BTreeNode *));
          node->leafs[0] = calloc(1, sizeof(struct BTreeNode));
          node->leafs[1] = calloc(1, sizeof(struct BTreeNode));
          node->leafs[0]->top = node;
          node->leafs[1]->top = node;

          const uint32_t index = (tree->max - 1) / 2;

          for (uint32_t i = 0; i < index; ++i) {
            struct KVPair *a = node->data[0];
            add_kv_to_node(node->leafs[0], a->key.value, get_kv_val(a, a->type), a->type);
            del_kv_from_node(node, a->key.value);

            struct KVPair *b = node->data[index - i];
            add_kv_to_node(node->leafs[1], b->key.value, get_kv_val(b, b->type), b->type);
            del_kv_from_node(node, b->key.value);
          }

          if (tree->max % 2 == 0) {
            struct KVPair *a = node->data[1];
            add_kv_to_node(node->leafs[1], a->key.value, get_kv_val(a, a->type), a->type);
            del_kv_from_node(node, a->key.value);
          }
        } else {
          node->top->leaf_count += 1;
          node->top->leafs = realloc(node->top->leafs, node->top->leaf_count * sizeof(struct BTreeNode *));
          node->top->leafs[node->top->leaf_count - 1] = calloc(1, sizeof(struct BTreeNode));
          node->top->leafs[node->top->leaf_count - 1]->top = node;

          const uint32_t index = (tree->max - 1) / 2;
          struct KVPair *tkv = node->data[index];
          add_kv_to_node(node->top, tkv->key.value, get_kv_val(tkv, tkv->type), tkv->type);
          del_kv_from_node(node, tkv->key.value);

          struct BTreeNode *empty = node->top->leafs[node->top->leaf_count - 1];
          const uint32_t leaf_index = leaf_at + 1;

          for (uint32_t i = leaf_index; i < node->top->leaf_count; ++i) {
            node->top->leafs[i] = node->top->leafs[i - 1];
          }

          node->top->leafs[leaf_index] = empty;

          for (uint32_t i = index + 1; i < tree->max; ++i) {
            struct KVPair *a = node->data[index];
            add_kv_to_node(empty, a->key.value, get_kv_val(a, a->type), a->type);
            del_kv_from_node(node, a->key.value);
          }
        }
      }
    }

    return res;
  }
}
