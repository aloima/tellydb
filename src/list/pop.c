#include "../../headers/telly.h"

#include <stddef.h>

void lpop_to_list(struct List *list) {
  if (list->size != 0) {
    struct ListNode *node = list->begin;

    if (list->size == 1) {
      free_list(list);
      // delete from btree
    } else {
      list->begin = list->begin->next;
      list->begin->prev = NULL;

      list->size -= 1;
      free_listnode(node);
    }
  }
}

void rpop_to_list(struct List *list) {
  if (list->size != 0) {
    struct ListNode *node = list->end;

    if (list->size == 1) {
      free_list(list);
      // delete from btree
    } else {
      list->end = list->end->prev;
      list->end->next = NULL;
    }

    list->size -= 1;
    free_listnode(node);
  }
}
