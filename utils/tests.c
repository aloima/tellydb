#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <hiredis/read.h>
#include <hiredis/hiredis.h>

#include <pthread.h>

#define HOST "127.0.0.1"
#define PORT 6379
#define LOADTESTS_CLIENT_COUNT 128

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

union ExpectedValue {
  int number;
  char *string;
};

void command_test(redisContext *ctx, const char *command, const int expected_type, union ExpectedValue expected_value) {
  const int command_len = strlen(command);

  char type_msg[28 + command_len], response_msg[29 + command_len];
  sprintf(type_msg, "Response type of '%s' command", command);
  sprintf(response_msg, "Response value of '%s' command", command);

  redisReply *reply = redisCommand(ctx, command);
  expect_int(type_msg, reply->type, expected_type);

  switch (expected_type) {
    case REDIS_REPLY_INTEGER:
      expect_int(response_msg, reply->integer, expected_value.number);
      break;

    case REDIS_REPLY_STATUS:
      expect_str(response_msg, reply->str, expected_value.string);
      break;

    case REDIS_REPLY_STRING:
      expect_str(response_msg, reply->str, expected_value.string);
      break;

    case REDIS_REPLY_NIL:
      break;

    default:
      break;
  }

  freeReplyObject(reply);
}

void command_tests() {
  redisContext *ctx = redisConnect(HOST, PORT);

  if (ctx == NULL || ctx->err) {
    puts("Error connecting to the server");
    return;
  }

  command_test(ctx, "GET key", REDIS_REPLY_NIL, (union ExpectedValue) {.number = 0});
  command_test(ctx, "SET key value", REDIS_REPLY_STATUS, (union ExpectedValue) {.string = "OK"});
  command_test(ctx, "GET key", REDIS_REPLY_STRING, (union ExpectedValue) {.string = "value"});
  command_test(ctx, "DEL key", REDIS_REPLY_INTEGER, (union ExpectedValue) {.number = 1});
  command_test(ctx, "GET key", REDIS_REPLY_NIL, (union ExpectedValue) {.number = 0});

  redisFree(ctx);
}

int succeed_clients = 0;
pthread_mutex_t mutex;
pthread_cond_t cond;

void *load_thread(void *arg) {
  const uint32_t id = *(((uint32_t *) arg));
  redisContext *ctx = redisConnect(HOST, PORT);

  if (ctx == NULL || ctx->err) {
    printf("Error connecting to the server for client #%d\n", id);
    pthread_exit(NULL);
  }

  uint32_t passed = 0;

  for (uint32_t i = 0; i < 100000; ++i) {
    redisReply *reply = redisCommand(ctx, "PING %d:%d", id, i);
    char str[32];
    sprintf(str, "%d:%d", id, i);

    if (reply->type == REDIS_REPLY_STRING && strcmp(reply->str, str) == 0) {
      passed += 1;
    }

    freeReplyObject(reply);
  }

  printf("%d of 100000 tests are passed for client #%d.\n", passed, id);

  pthread_mutex_lock(&mutex);
  succeed_clients += 1;
  if (succeed_clients == LOADTESTS_CLIENT_COUNT) pthread_cond_signal(&cond);
  pthread_mutex_unlock(&mutex);

  pthread_exit(NULL);
}

void load_tests(uint32_t count) {
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond, NULL);

  pthread_t threads[count];
  uint32_t *ids = malloc(count * sizeof(uint32_t));

  for (uint32_t i = 0; i < count; ++i) {
    ids[i] = i;
    pthread_create(&threads[i], NULL, load_thread, &ids[i]);
    pthread_detach(threads[i]);
  }

  while (pthread_cond_wait(&cond, &mutex));
  free(ids);
}

int main() {
  command_tests();
  load_tests(LOADTESTS_CLIENT_COUNT);

  return 0;
}
