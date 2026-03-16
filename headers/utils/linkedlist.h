#pragma once

#include <stdint.h>

typedef struct LinkedListNode {
  uint8_t *data;
  struct LinkedListNode *prev, *next;
} LinkedListNode;

typedef enum {
  LL_BACK,
  LL_FRONT,
  LL_DOUBLE
} LLSearchDirection;

LinkedListNode *ll_create_node(void *data);
LinkedListNode *ll_insert_back(LinkedListNode *node, void *data);
LinkedListNode *ll_insert_front(LinkedListNode *node, void *data);
LinkedListNode *ll_search_node(LinkedListNode *node, const LLSearchDirection dir, void *external, bool (*cmp)(void *data, void *external));
void ll_free_each(LinkedListNode *node, void (*free_data)(void *data));
