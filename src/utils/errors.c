#include "../telly.h"

#include <stdio.h>
#include <stdlib.h>

void client_error() {
  fputs("invalid client, please use redis-cli or its full implementation\n", stderr);
  exit(EXIT_FAILURE);
}
