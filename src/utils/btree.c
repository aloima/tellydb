// B-Tree implementation

#include "../../headers/telly.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct BTree *create_btree(const uint32_t max) {
  struct BTree *tree = malloc(sizeof(struct BTree));
  tree->max = max;
  tree->root = NULL;

  return tree;
}

void set_kv(struct KVPair *pair, char *key, void *value, uint32_t type) {
  pair->type = type;
  set_string(&pair->key, key, -1);

  switch (type) {
    case TELLY_STR:
      set_string(&pair->value.string, value, -1);
      break;

    case TELLY_INT:
      pair->value.integer = *((int *) value);
      break;

    case TELLY_BOOL:
      pair->value.boolean = *((bool *) value);
      break;

    case TELLY_NULL:
      pair->value.null = NULL;
      break;
  }
}

void *get_kv_val(struct KVPair *pair, uint32_t type) {
  switch (type) {
    case TELLY_STR:
      return pair->value.string.value;

    case TELLY_INT:
      return &pair->value.integer;

    case TELLY_BOOL:
      return &pair->value.boolean;

    case TELLY_NULL:
      return NULL;
  }

  return NULL;
}

/*
  1 2 3 4 5
  if index=1 then
  1 _ 2 3 4 5
*/
void move_kv(struct BTreeNode *node, uint32_t index) {
  uint32_t bound = node->size - 2;

  for (uint32_t i = bound; index <= i; --i) {
    struct KVPair *a = &node->data[i];
    struct KVPair *b = &node->data[i + 1];

    b->type = a->type;
    set_string(&b->key, a->key.value, a->key.len);

    switch (a->type) {
      case TELLY_STR:
        set_string(&b->value.string, a->value.string.value, a->value.string.len);
        break;

      case TELLY_INT:
        b->value.integer = a->value.integer;
        break;

      case TELLY_BOOL:
        b->value.boolean = a->value.boolean;
        break;

      case TELLY_NULL:
        b->value.null = NULL;
        break;
    }
  }
}

void add_kv_to_node(struct BTreeNode *node, char *key, void *value, uint32_t type) {
  node->size += 1;

  if (node->size == 1) {
    node->data = calloc(1, sizeof(struct KVPair));
    set_kv(&node->data[0], key, value, type);
  } else {
    node->data = realloc(node->data, node->size * sizeof(struct KVPair));
    memset(&node->data[node->size - 1], 0, sizeof(struct KVPair));

    const char c = key[0];

    if (node->data[node->size - 2].key.value[0] <= c) {
      set_kv(&node->data[node->size - 1], key, value, type);
    } else if (c <= node->data[0].key.value[0]) {
      move_kv(node, 0);
      set_kv(&node->data[0], key, value, type);
    } else {
      const uint32_t bound = node->size - 2;

      for (uint32_t i = 0; i < bound; ++i) {
        if (node->data[i].key.value[0] <= c && c <= node->data[i + 1].key.value[0]) {
          move_kv(node, i + 1);
          set_kv(&node->data[i + 1], key, value, type);
          break;
        }
      }
    }
  }
}

void del_kv_from_node(struct BTreeNode *node, char *key) {
  for (uint32_t i = 0; i < node->size; ++i) {
    if (streq(node->data[i].key.value, key)) {
      const uint32_t bound = node->size - 1;

      for (uint32_t j = i; j < bound; ++j) {
        struct KVPair *a = &node->data[j];
        struct KVPair *b = &node->data[j + 1];

        a->type = b->type;
        set_string(&a->key, b->key.value, b->key.len);

        if (a->type == TELLY_STR && b->type != TELLY_STR) {
          free(a->value.string.value);
        }

        switch (b->type) {
          case TELLY_STR:
            set_string(&a->value.string, b->value.string.value, b->value.string.len);
            break;

          case TELLY_INT:
            a->value.integer = b->value.integer;
            break;

          case TELLY_BOOL:
            a->value.boolean = b->value.boolean;
            break;

          case TELLY_NULL:
            a->value.null = NULL;
            break;
        }
      }

      node->size -= 1;
      node->data = realloc(node->data, node->size * sizeof(struct KVPair));
      break;
    }
  }
}

void insert_kv_to_btree(struct BTree *tree, char *key, void *value, uint32_t type) {
  if (tree->root == NULL) {
    tree->root = calloc(1, sizeof(struct BTreeNode));
    add_kv_to_node(tree->root, key, value, type);
  } else {
    struct BTreeNode *node = tree->root;
    char c = key[0];

    if (node->leaf_count != 0) {
      for (uint32_t i = 0; i < node->size; ++i) {
        if (c <= node->data[i].key.value[0]) {
          node = &node->leafs[i];
          break;
        }
      }

      if (node->top == NULL) {
        node = &node->leafs[node->leaf_count - 1];
      }
    }

    add_kv_to_node(node, key, value, type);

    if (node->size == tree->max) {
      if (node->leaf_count == 0) {
        node->leaf_count = 2;
        node->leafs = calloc(node->leaf_count, sizeof(struct BTreeNode));
        node->leafs[0].top = node;
        node->leafs[1].top = node;

        const uint32_t index = (tree->max - 1) / 2;

        for (uint32_t i = 0; i < index; ++i) {
          struct KVPair a = node->data[0];
          add_kv_to_node(&node->leafs[0], a.key.value, get_kv_val(&a, a.type), a.type);
          del_kv_from_node(node, a.key.value);

          struct KVPair b = node->data[index - i];
          add_kv_to_node(&node->leafs[1], b.key.value, get_kv_val(&b, b.type), b.type);
          del_kv_from_node(node, b.key.value);
        }

        if (tree->max % 2 == 0) {
          struct KVPair a = node->data[1];
          add_kv_to_node(&node->leafs[1], a.key.value, get_kv_val(&a, a.type), a.type);
          del_kv_from_node(node, a.key.value);
        }
      } else {
        node->leaf_count += 1;
        node->leafs = realloc(node->leafs, node->leaf_count * sizeof(struct BTreeNode));
        memset(&node->leafs[node->leaf_count - 1], 0, sizeof(struct BTreeNode));
        node->leafs[node->leaf_count - 1].top = node;
      }
    }
  }
}
