#include "../../headers/telly.h"

#include <stdint.h>
#include <stdlib.h>

struct KVPair *add_kv_to_node(struct BTreeNode *node, char *key, void *value, enum TellyTypes type) {
  if (node->size == 0) {
    node->size += 1;
    node->data = malloc(sizeof(struct KVPair *));
    node->data[0] = calloc(1, sizeof(struct KVPair));
    set_kv(node->data[0], key, value, type);

    return node->data[0];
  } else {
    const uint32_t index = find_index_of_kv(node, key);

    node->size += 1;
    node->data = realloc(node->data, node->size * sizeof(struct KVPair *));

    move_last_kv_to(node, index);
    node->data[index] = calloc(1, sizeof(struct KVPair));
    set_kv(node->data[index], key, value, type);

    return node->data[index];
  }

  return NULL;
}

struct KVPair *insert_kv_to_btree(struct BTree *tree, char *key, void *value, enum TellyTypes type) {
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

          const uint32_t index = (tree->max - 1) / 2;

          node->leafs[0]->top = node;
          node->leafs[1]->top = node;

          node->leafs[0]->size = index;
          node->leafs[0]->data = malloc(node->leafs[0]->size * sizeof(struct KVPair *));

          if (tree->max % 2 == 0) {
            node->leafs[1]->size = index + 1;
            node->leafs[1]->data = malloc(node->leafs[1]->size * sizeof(struct KVPair *));

            memcpy(node->leafs[0]->data, node->data, index * sizeof(struct BTreeNode *));
            memcpy(node->leafs[1]->data, node->data + index + 1, index * sizeof(struct BTreeNode *));

            node->leafs[1]->data[index] = node->data[index + index + 1];
          } else {
            node->leafs[1]->size = index;
            node->leafs[1]->data = malloc(node->leafs[1]->size * sizeof(struct KVPair *));

            memcpy(node->leafs[0]->data, node->data, index * sizeof(struct BTreeNode *));
            memcpy(node->leafs[1]->data, node->data + index + 1, index * sizeof(struct BTreeNode *));
          }

          node->data[0] = node->data[index];
          node->data = realloc(node->data, sizeof(struct BTreeNode *));
          node->size = 1;
        } else {
          node->top->leaf_count += 1;
          node->top->leafs = realloc(node->top->leafs, node->top->leaf_count * sizeof(struct BTreeNode *));
          node->top->leafs[node->top->leaf_count - 1] = malloc(sizeof(struct BTreeNode));

          const uint32_t leaf_index = leaf_at + 1;
          const uint32_t index = (tree->max - 1) / 2;
          struct KVPair *tkv = node->data[index];

          const uint32_t tkv_index = find_index_of_kv(node->top, tkv->key.value);
          node->top->size += 1;
          node->top->data = realloc(node->top->data, node->top->size * sizeof(struct KVPair *));
          move_last_kv_to(node->top, tkv_index);
          node->top->data[tkv_index] = tkv;

          memcpy(node->data + index, node->data + index + 1, (node->size - index - 1) * sizeof(struct KVPair *));
          node->size -= 1;
          node->data = realloc(node->data, node->size * sizeof(struct KVPair *));

          struct BTreeNode *empty = node->top->leafs[node->top->leaf_count - 1];
          empty->top = node;
          empty->leafs = NULL;
          empty->leaf_count = 0;
          empty->size = tree->max - index - 1;
          empty->data = malloc(empty->size * sizeof(struct KVPair *));
          memcpy(empty->data, node->data + index, empty->size * sizeof(struct KVPair *));

          memcpy(node->top->leafs + leaf_index + 1, node->top->leafs + leaf_index, (node->top->leaf_count - leaf_index - 1) * sizeof(struct BTreeNode *));
          node->top->leafs[leaf_index] = empty;

          node->size -= empty->size;
          node->data = realloc(node->data, (tree->max - index) * sizeof(struct KVPair *));
        }
      }
    }

    return res;
  }
}
