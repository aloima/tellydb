#pragma once

#include <stdint.h>

typedef struct LinkedListNode {
  uint8_t *data;
  struct LinkedListNode *prev, *next;
} LinkedListNode;

typedef struct LinkedList {
  LinkedListNode *begin, *end;
  uint64_t size;
} LinkedList;

typedef enum {
  LL_BACK,
  LL_FRONT
} LLSearchDirection;

LinkedList *ll_create(void *initial_data);
LinkedListNode *ll_insert_back(LinkedList *list, void *data);
LinkedListNode *ll_insert_front(LinkedList *list, void *data);
LinkedListNode *ll_search_node(LinkedList *list, const LLSearchDirection dir, void *external, bool (*cmp)(void *data, void *external));
void ll_free(LinkedList *list, void (*free_data)(void *data));
