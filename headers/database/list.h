#pragma once

#include "../utils/utils.h"

#include <stdint.h>

typedef struct DatabaseListNode {
  enum TellyTypes type;
  void *data;
} DatabaseListNode;

void free_databaselistnode(void *data);
