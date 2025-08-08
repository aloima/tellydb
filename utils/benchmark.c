#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <pthread.h>

#include <hiredis/hiredis.h>

#define NUM_REQUESTS 100000  // Number of SET/GET/PING requests per server
#define LOCALHOST "127.0.0.1"

typedef struct {
  const char *host;
  int port;
  double set_time;
  double get_time;
  double ping_time;
} BenchmarkResult;

double get_time_ms() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000.0 + ts.tv_nsec / 1.0e6;
}

void *benchmark(void *arg) {
  BenchmarkResult *result = (BenchmarkResult *) arg;
  redisContext *ctx = redisConnect(result->host, result->port);
  
  if (ctx == NULL || ctx->err) {
    printf("Error connecting to %s:%d\n", result->host, result->port);
    return NULL;
  }

  redisReply *reply;
  char key[32], value[32];
  
  double start = get_time_ms();

  for (int i = 0; i < NUM_REQUESTS; i++) {
    snprintf(key, sizeof(key), "key%d", i);
    snprintf(value, sizeof(value), "value%d", i);
    reply = redisCommand(ctx, "SET %s %s", key, value);
    if (reply) freeReplyObject(reply);
  }

  result->set_time = get_time_ms() - start;

  start = get_time_ms();

  for (int i = 0; i < NUM_REQUESTS; i++) {
    snprintf(key, sizeof(key), "key%d", i);
    reply = redisCommand(ctx, "GET %s", key);
    if (reply) freeReplyObject(reply);
  }

  result->get_time = get_time_ms() - start;

  start = get_time_ms();

  for (int i = 0; i < NUM_REQUESTS; i++) {
    reply = redisCommand(ctx, "PING", key);
    if (reply) freeReplyObject(reply);
  }

  result->ping_time = get_time_ms() - start;

  redisFree(ctx);
  return NULL;
}

int main() {
  BenchmarkResult results[] = {
    {LOCALHOST, 6379, 0, 0, 0},
    {LOCALHOST, 6380, 0, 0, 0}
  };

  const uint32_t count = (sizeof(results) / sizeof(BenchmarkResult));
  pthread_t threads[count];

  for (uint32_t i = 0; i < count; ++i) {
    pthread_create(&threads[i], NULL, benchmark, &results[i]);
  }

  for (uint32_t i = 0; i < count; ++i) {
    pthread_join(threads[i], NULL);
  }

  printf("Benchmark results (%d operations per server):\n", NUM_REQUESTS);

  for (uint32_t i = 0; i < count; ++i) {
    printf("%s:%d test: SET=%.2f ms, GET=%.2f ms, PING=%.2f ms\n", results[i].host, results[i].port, results[i].set_time, results[i].get_time, results[i].ping_time);
  }

  return 0;
}
