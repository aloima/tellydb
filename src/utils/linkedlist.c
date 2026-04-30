#include <telly.h>

static inline LinkedListNode *ll_create_node(void *data) {
  LinkedListNode *node = malloc(sizeof(LinkedListNode));
  node->data = data;
  node->prev = NULL;
  node->next = NULL;

  return node;
}

LinkedList *ll_create() {
  LinkedList *list = malloc(sizeof(LinkedList));
  if (list == NULL) return NULL;

  list->begin = NULL;
  list->end = NULL;
  list->size = 0;

  return list;
}

LinkedListNode *ll_insert_back(LinkedList *list, void *data) {
  LinkedListNode *node = ll_create_node(data);
  if (node == NULL) return NULL;

  if (list->size == 0) {
    list->begin = node;
    list->end = node;
    list->size = 1;
    return node;
  }

  LinkedListNode *old_node = list->end;
  list->end->next = node;

  list->end = node;
  list->end->prev = old_node;

  list->size += 1;
  return node;
}

LinkedListNode *ll_insert_front(LinkedList *list, void *data) {
  LinkedListNode *node = ll_create_node(data);
  if (node == NULL) return NULL;

  if (list->size == 0) {
    list->begin = node;
    list->end = node;
    list->size = 1;
    return node;
  }

  LinkedListNode *old_node = list->begin;
  list->begin->prev = node;

  list->begin = node;
  list->begin->next = old_node;

  list->size += 1;
  return node;
}

LinkedListNode *ll_search_node(LinkedList *list, const LLSearchDirection dir, void *external, bool (*cmp)(void *data, void *external)) {
  LinkedListNode *front = list->begin;
  LinkedListNode *back = list->end;

  switch (dir) {
    case LL_BACK:
      while (back != NULL) {
        if (cmp(back->data, external)) return back;
        back = back->next;
      }

      return NULL;

    case LL_FRONT:
      while (front != NULL) {
        if (cmp(front->data, external)) return front;
        front = front->prev;
      }

      return NULL;
  }
}

void ll_free(LinkedList *list, void (*free_data)(void *data)) {
  LinkedListNode *front = list->begin;
  LinkedListNode *back = list->end;

  // Checking free_data existence is unperformant, but it's readable
  while (front != NULL || back != NULL) {
    if (front) {
      if (free_data) free_data(front->data);
      LinkedListNode *tmp = front;
      front = front->prev;
      free(tmp);
    }

    if (back) {
      if (free_data) free_data(back->data);
      LinkedListNode *tmp = back;
      back = back->next;
      free(tmp);
    }
  }
}
