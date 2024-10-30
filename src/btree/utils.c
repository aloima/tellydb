#include "../../headers/btree.h"

#include <stdint.h>

uint32_t find_node_of_index(struct BTreeNode **result, struct BTreeNode *search, const uint64_t index) {
  if (search->children) {
    for (uint32_t i = 0; i < search->size; ++i) {
      const struct BTreeValue *value = search->data[i];

      if (value->index < index) {
        return find_node_of_index(result, search->children[i], index);
      } else if (value->index == index) {
        *result = search;
        return i;
      }
    }

    return find_node_of_index(result, search->children[search->size], index);
  }

  *result = search;

  for (uint32_t i = 0; i < search->size; ++i) {
    if (search->data[i]->index <= index) return i;
  }

  return search->size;
}
