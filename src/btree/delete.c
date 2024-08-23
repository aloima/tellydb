#include "../../headers/telly.h"

#include <stdlib.h>
#include <string.h>

void del_kv_from_node(struct BTreeNode *node, char *key) {
  for (uint32_t i = 0; i < node->size; ++i) {
    if (streq(node->data[i]->key.value, key)) {
      struct KVPair *del_addr = node->data[i];
      memcpy(node->data + i, node->data + i + 1, (node->size - i - 1) * sizeof(struct KVPair *));

      free(del_addr->key.value);
      if (del_addr->type == TELLY_STR) free(del_addr->value.string.value);
      free(del_addr);

      node->size -= 1;
      node->data = realloc(node->data, node->size * sizeof(struct KVPair *));
      break;
    }
  }
}
