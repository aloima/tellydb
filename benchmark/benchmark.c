#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <hiredis/hiredis.h>

int main(int argc, char **argv) {
  if (argc != 3) {
    puts("write ip and port");
    return 0;
  }

  printf("ip: %s\nport: %s\n", argv[1], argv[2]);

  redisContext *c = redisConnect(argv[1], atoi(argv[2]));

  if (c->err) {
    printf("error: %s\n", c->errstr);
    return 1;
  }

  for (uint32_t i = 0; i < 10000; ++i) {
    freeReplyObject(redisCommand(c, "SET %d %d-value", i, i));
  }

  redisFree(c);
}
