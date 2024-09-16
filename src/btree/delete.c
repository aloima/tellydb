#include "../../headers/telly.h"

#include <stdint.h>
#include <stdlib.h>

void del_kv_from_node(struct BTreeNode *node, char *key) {
  const uint32_t at = find_index_of_kv(node, key);
  if (at > node->size) return;

  struct KVPair *pair = node->data[at];

  if (node->size == 1 && streq(pair->key.value, key)) {
    free_kv(pair);
    free(node->data);

    node->data = NULL;
    node->size = 0;
  } else if (streq(pair->key.value, key)) {
    move_kv(node, at, node->size - 1);
    free_kv(pair);

    node->size -= 1;
    node->data = realloc(node->data, node->size * sizeof(struct KVPair *));
  }
}
