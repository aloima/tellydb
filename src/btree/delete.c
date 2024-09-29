#include "../../headers/telly.h"

#include <stdint.h>
#include <stdlib.h>

void del_kv_from_node(struct BTreeNode *node, const char *key) {
  const uint32_t at = find_index_of_kv(node, key);
  if (at >= node->size) return;

  struct KVPair *kv = node->data[at];

  if (streq(kv->key.value, key)) {
    if (node->size == 1) {
      free_kv(kv);
      free(node->data);

      node->data = NULL;
      node->size = 0;
    } else {
      move_kv(node, at, node->size - 1);
      free_kv(kv);

      node->size -= 1;
      node->data = realloc(node->data, node->size * sizeof(struct KVPair *));
    }
  }
}
