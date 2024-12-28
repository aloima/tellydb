#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include <hiredis/hiredis.h>

int main() {
  redisContext *c;

  c = redisConnect("127.0.0.1", 6379);

  if (c->err) {
    printf("error: %s\n", c->errstr);
    return 1;
  }

  clock_t start_time = clock();
  uint32_t ops = 0;

  for (uint32_t i = 0; i < 1e7; ++i) {
    freeReplyObject(redisCommand(c,"GET string"));
    ops += 1;
  }

  double elapsed_time = (double)(clock() - start_time) / CLOCKS_PER_SEC;
  printf("Done in %f seconds, %f ops/sec\n", elapsed_time, (ops / elapsed_time));

  redisFree(c);
}
