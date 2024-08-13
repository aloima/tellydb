// B-Tree implementation

#include "../../headers/telly.h"

#include <stdint.h>
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
  1 2 3 4 5 _
  if index=1 then
  1 _ 2 3 4 5
*/
void move_kv(struct BTreeNode *node, int32_t index) {
  int32_t bound = node->size - 2;

  for (int32_t i = bound; index <= i; --i) {
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

  struct KVPair *pair = &node->data[index];

  if (pair->type == TELLY_STR) {
    free(pair->value.string.value);
    pair->value.string = (string_t) {0};
  }

  free(pair->key.value);
  pair->key = (string_t) {0};
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

        if (a->type == TELLY_STR && b->type != TELLY_STR) {
          free(a->value.string.value);
          a->value.string = (string_t) {0};
        }

        a->type = b->type;
        set_string(&a->key, b->key.value, b->key.len);

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

      struct KVPair last_pair = node->data[node->size - 1];
      free(last_pair.key.value);
      if (last_pair.type == TELLY_STR) free(last_pair.value.string.value);

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
          node = node->leafs[i];
          break;
        }
      }

      if (node->top == NULL) {
        node = node->leafs[node->leaf_count - 1];
      }
    }

    add_kv_to_node(node, key, value, type);

    if (node->size == tree->max) {
      if (node->leaf_count == 0) {
        if ((node->top && node->top->size == tree->max) || !node->top) {
          node->leaf_count = 2;
          node->leafs = malloc(node->leaf_count * sizeof(struct BTreeNode *));
          node->leafs[0] = calloc(1, sizeof(struct BTreeNode));
          node->leafs[1] = calloc(1, sizeof(struct BTreeNode));
          node->leafs[0]->top = node;
          node->leafs[1]->top = node;

          const uint32_t index = (tree->max - 1) / 2;

          for (uint32_t i = 0; i < index; ++i) {
            struct KVPair a = node->data[0];
            add_kv_to_node(node->leafs[0], a.key.value, get_kv_val(&a, a.type), a.type);
            del_kv_from_node(node, a.key.value);

            struct KVPair b = node->data[index - i];
            add_kv_to_node(node->leafs[1], b.key.value, get_kv_val(&b, b.type), b.type);
            del_kv_from_node(node, b.key.value);
          }

          if (tree->max % 2 == 0) {
            struct KVPair a = node->data[1];
            add_kv_to_node(node->leafs[1], a.key.value, get_kv_val(&a, a.type), a.type);
            del_kv_from_node(node, a.key.value);
          }
        } else {
          node->top->leaf_count += 1;
          node->top->leafs = realloc(node->top->leafs, node->top->leaf_count * sizeof(struct BTreeNode *));
          node->top->leafs[node->top->leaf_count - 1] = calloc(1, sizeof(struct BTreeNode));
          node->top->leafs[node->top->leaf_count - 1]->top = node;

          const uint32_t index = ((tree->max % 2 == 1) ? tree->max : (tree->max - 1)) / 2;
          struct KVPair tkv = node->data[index];
          add_kv_to_node(node->top, tkv.key.value, get_kv_val(&tkv, tkv.type), tkv.type);
          del_kv_from_node(node, tkv.key.value);

          for (uint32_t i = index + 1; i < tree->max; ++i) {
            struct KVPair a = node->data[index];
            add_kv_to_node(node->top->leafs[node->top->leaf_count - 1], a.key.value, get_kv_val(&a, a.type), a.type);
            del_kv_from_node(node, a.key.value);
          }
        }
      }
    }
  }
}

struct KVPair find_kv_from_node(struct BTreeNode *node, const char *key) {
  const char *first = node->data[0].key.value;
  const char c = key[0];

  if (streq(first, key)) {
    return node->data[0];
  } else if (node->leaf_count != 0 && c <= first[0]) {
    return find_kv_from_node(node->leafs[0], key);
  } else if (node->leaf_count != 0) {
    for (uint32_t i = 0; i < node->size; ++i) {
      struct KVPair pair = node->data[i];
      const char *pair_key = pair.key.value;

      if (streq(key, pair_key)) {
        return pair;
      } else if (c <= pair_key[0]) {
        return find_kv_from_node(node->leafs[i], key);
      }
    }
  } else {
    for (uint32_t i = 1; i < node->size; ++i) {
      struct KVPair pair = node->data[i];

      if (streq(pair.key.value, key)) {
        return pair;
      }
    }
  }

  return (struct KVPair) {0};
}

struct KVPair find_kv_from_btree(struct BTree *tree, const char *key) {
  return find_kv_from_node(tree->root, key);
}

void free_btree_node(struct BTreeNode *node) {
  for (uint32_t i = 0; i < node->size; ++i) {
    struct KVPair pair = node->data[i];
    free(pair.key.value);

    if (pair.type == TELLY_STR) {
      free(pair.value.string.value);
    }
  }

  for (uint32_t i = 0; i < node->leaf_count; ++i) {
    free_btree_node(node->leafs[i]);
  }

  if (node->leaf_count != 0) free(node->leafs);
  free(node->data);
  free(node);
}

void free_btree(struct BTree *tree) {
  free_btree_node(tree->root);
  free(tree);
}
