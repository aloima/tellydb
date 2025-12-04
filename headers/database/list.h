#pragma once

#include "../utils/utils.h"

#include <stdint.h>

struct ListNode {
  void *value;
  enum TellyTypes type;

  struct ListNode *prev;
  struct ListNode *next;
};

struct List {
  uint32_t size;
  struct ListNode *begin;
  struct ListNode *end;
};

struct List *create_list();
struct ListNode *create_listnode(void *value, enum TellyTypes type);
void free_listnode(struct ListNode *node);
void free_list(struct List *list);
