#include "../../headers/telly.h"

#include <stdint.h>

uint32_t find_node_of_index(struct BTreeNode **result, struct BTreeNode *search, const uint64_t index) {
  if (search->children) {
    for (uint32_t i = 0; i < search->size; ++i) {
      const struct BTreeValue *value = search->data[i];

      if (index < value->index) {
        return find_node_of_index(result, search->children[i], index);
      } else if (index == value->index) {
        *result = search;
        return i;
      }
    }

    return find_node_of_index(result, search->children[search->size], index);
  }

  *result = search;

  for (uint32_t i = 0; i < search->size; ++i) {
    if (index <= search->data[i]->index) return i;
  }

  return search->size;
}
