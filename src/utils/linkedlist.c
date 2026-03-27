#include <telly.h>

#include <stdlib.h>

LinkedListNode *ll_create_node(void *data) {
  LinkedListNode *node = malloc(sizeof(LinkedListNode));
  node->data = data;
  node->prev = NULL;
  node->next = NULL;

  return node;
}

LinkedListNode *ll_insert_back(LinkedListNode *node, void *data) {
  while (node->next != NULL) {
    node = node->next;
  }

  LinkedListNode *source = ll_create_node(data);
  source->prev = node;
  node->next = source;

  return source;
}

LinkedListNode *ll_insert_front(LinkedListNode *node, void *data) {
  while (node->prev != NULL) {
    node = node->prev;
  }

  LinkedListNode *source = ll_create_node(data);
  source->next = node;
  node->prev = source;

  return source;
}

LinkedListNode *ll_search_node(LinkedListNode *node, const LLSearchDirection dir, void *external, bool (*cmp)(void *data, void *external)) {
  LinkedListNode *front = node;
  LinkedListNode *back = node;

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

    case LL_DOUBLE:
      while (front != NULL || back != NULL) {
        if (front) {
          if (cmp(front->data, external)) return front;
          front = front->prev;
        }

        if (back) {
          if (cmp(back->data, external)) return back;
          back = back->next;
        }
      }

      return NULL;
  }
}

void ll_free_each(LinkedListNode *node, void (*free_data)(void *data)) {
  LinkedListNode *front = node->prev;
  LinkedListNode *back = node->next;

  free_data(node->data);
  free(node);

  while (front != NULL || back != NULL) {
    if (front) {
      free_data(front->data);
      LinkedListNode *tmp = front;
      front = front->prev;
      free(tmp);
    }

    if (back) {
      free_data(back->data);
      LinkedListNode *tmp = back;
      back = back->next;
      free(tmp);
    }
  }
}
