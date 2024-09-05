#include "../../headers/telly.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void del_kv_from_node(struct BTreeNode *node, char *key) {
  for (uint32_t i = 0; i < node->size; ++i) {
    struct KVPair *pair = node->data[i];

    if (streq(pair->key.value, key)) {
      memcpy(node->data + i, node->data + i + 1, (node->size - i - 1) * sizeof(struct KVPair *));

      free(pair->key.value);
      if (pair->type == TELLY_STR) free(pair->value.string.value);
      free(pair);

      node->size -= 1;
      node->data = realloc(node->data, node->size * sizeof(struct KVPair *));
      break;
    }
  }
}
