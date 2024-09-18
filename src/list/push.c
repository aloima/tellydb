#include "../../headers/telly.h"

struct ListNode *lpush_to_list(struct List *list, void *value, enum TellyTypes type) {
  struct ListNode *node = create_listnode(value, type);
  node->next = list->begin;
  list->begin = node;
  list->size += 1;

  if (list->size == 1) {
    list->end = node;
  } else {
    node->next->prev = node;
  }

  return node;
}

struct ListNode *rpush_to_list(struct List *list, void *value, enum TellyTypes type) {
  struct ListNode *node = create_listnode(value, type);
  node->prev = list->end;
  list->end = node;
  list->size += 1;

  if (list->size == 1) {
    list->begin = node;
  } else {
    node->prev->next = node;
  }

  return node;
}
