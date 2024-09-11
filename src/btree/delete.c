#include "../../headers/telly.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void del_kv_from_node(struct BTreeNode *node, char *key) {
  const uint32_t at = find_index_of_kv(node, key);

  if (node->size == 1 && streq(node->data[0]->key.value, key)) {
    free(node->data);
    node->data = NULL;
    node->size = 0;
  } else if (streq(node->data[at]->key.value, key)) {
    move_kv(node, at, node->size - 1);
    node->size -= 1;
    node->data = realloc(node->data, node->size * sizeof(struct KVPair *));
  }
}
