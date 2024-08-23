// B-Tree implementation

#include "../../headers/telly.h"

#include <stdint.h>
#include <stdlib.h>

struct BTree *create_btree(const uint32_t max) {
  struct BTree *tree = malloc(sizeof(struct BTree));
  tree->max = max;
  tree->root = NULL;

  return tree;
}
