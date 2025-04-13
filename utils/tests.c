#include <stdio.h>
#include <string.h>

#include <hiredis/read.h>
#include <hiredis/hiredis.h>

#define HOST "127.0.0.1"
#define PORT 6379

static redisContext *ctx;

void expect_str(const char *name, const char *value, const char *expected) {
  if (strcmp(value, expected) == 0) {
    printf("%s test is passed.\n", name);
  } else {
    printf("[!] %s test is failed.\n", name);
  }
}

void expect_int(const char *name, const int value, const int expected) {
  if (value == expected) {
    printf("%s test is passed.\n", name);
  } else {
    printf("[!] %s test is failed.\n", name);
  }
}

void command_tests() {
  redisReply *reply;

  reply = redisCommand(ctx, "SET key value");
  expect_int("Type of SET command without arguments", reply->type, REDIS_REPLY_STATUS);
  expect_str("Value of SET command without arguments", reply->str, "OK");
  freeReplyObject(reply);

  reply = redisCommand(ctx, "GET key");
  expect_int("Type of GET command for existed key", reply->type, REDIS_REPLY_STRING);
  expect_str("Value of GET command for existed key", reply->str, "value");
  freeReplyObject(reply);
}

int main() {
  ctx = redisConnect(HOST, PORT);
  if (ctx == NULL || ctx->err) {
    puts("Error connecting to the server");
    return 1;
  }

  command_tests();

  redisFree(ctx);
  return 0;
}
