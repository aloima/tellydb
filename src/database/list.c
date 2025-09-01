#include <telly.h>

#include <stdint.h>
#include <stdlib.h>

struct List *create_list() {
  return calloc(1, sizeof(struct List));
}

struct ListNode *create_listnode(void *value, enum TellyTypes type) {
  struct ListNode *node = malloc(sizeof(struct ListNode));
  node->prev = NULL;
  node->next = NULL;
  node->type = type;
  node->value = value;

  return node;
}

void free_listnode(struct ListNode *node) {
  if (node->type == TELLY_STR) {
    string_t *string = node->value;
    free(string->value);
  }

  if (node->type != TELLY_NULL) {
    free(node->value);
  }

  free(node);
}

void free_list(struct List *list) {
  struct ListNode *node = list->begin;

  while (node) {
    struct ListNode *_node = node;
    node = node->next;
    free_listnode(_node);
  }

  free(list);
}
