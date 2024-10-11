#include "../../headers/database.h"
#include "../../headers/utils.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

struct List *create_list() {
  return calloc(1, sizeof(struct List));
}

struct ListNode *create_listnode(void *value, enum TellyTypes type) {
  struct ListNode *node = malloc(sizeof(struct ListNode));
  node->prev = NULL;
  node->next = NULL;

  switch (node->type = type) {
    case TELLY_STR: {
      const uint32_t len = strlen(value);
      const uint32_t size = len + 1;
      node->value.string.len = len;
      node->value.string.value = malloc(size);
      memcpy(node->value.string.value, value, size);
      break;
    }

    case TELLY_INT:
      node->value.integer = *((int *) value);
      break;

    case TELLY_BOOL:
      node->value.boolean = *((bool *) value);
      break;

    default:
      break;
  }

  return node;
}

void free_listnode(struct ListNode *node) {
  if (node->type == TELLY_STR) free(node->value.string.value);
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
