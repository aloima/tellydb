#include "utils.h"
#include "database.h"

#ifndef LIST_H_
  #define LIST_H_

  struct ListNode {
    value_t value;
    enum TellyTypes type;

    struct ListNode *prev;
    struct ListNode *next;
  };

  struct List {
    uint64_t size;
    struct ListNode *begin;
    struct ListNode *end;
  };

  struct List *create_list();
  struct ListNode *create_listnode(void *value, enum TellyTypes type);
  void free_listnode(struct ListNode *node);
  void free_list(struct List *list);

  struct ListNode *lpush_to_list(struct List *list, void *value, enum TellyTypes type);
  struct ListNode *rpush_to_list(struct List *list, void *value, enum TellyTypes type);

  void lpop_to_list(struct List *list);
  void rpop_to_list(struct List *list);
#endif
