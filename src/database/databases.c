#include "../../headers/telly.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

static struct LinkedListNode *start = NULL;
static struct LinkedListNode *end = NULL;
static struct Database *main = NULL;
static uint32_t database_count = 0;

struct Database *create_database(const string_t name) {
  struct Database *database = malloc(sizeof(struct Database));;
  struct LinkedListNode *node = malloc(sizeof(struct LinkedListNode));
  database_count += 1;

  if (database_count != 1) {
    end->next = node;
    end = node;
    node->data = database;
    node->next = NULL;
  } else {
    start = node;
    end = node;
    node->data = database;
    node->next = NULL;
  }

  database->name = (string_t) {
    .value = malloc(name.len),
    .len = name.len
  };
  memcpy(database->name.value, name.value, name.len);

  database->id = hash(name.value, name.len);
  database->cache = create_btree(5);

  return database;
}

void set_main_database(struct Database *database) {
  main = database;
}

struct Database *get_main_database() {
  return main;
}

struct BTree *get_cache_of_database(const string_t name) {
  const uint64_t target = hash(name.value, name.len);
  struct LinkedListNode *node = start;

  while (node) {
    struct Database *database = node->data;
    if (database->id == target) return database->cache;
    node = node->next;
  }

  return NULL;
}

struct LinkedListNode *get_database_node() {
  return start;
}

bool rename_database(const string_t old_name, const string_t new_name) {
  const uint64_t target = hash(old_name.value, old_name.len);
  struct LinkedListNode *node = start;

  while (node) {
    struct Database *database = node->data;

    if (database->id == target) {
      database->id = hash(new_name.value, new_name.len);
      return true;
    }

    node = node->next;
  }

  return false;
}

static void free_database(struct Database *database) {
  free_btree(database->cache, (void (*)(void *)) free_kv);
  free(database->name.value);
  free(database);
}

void free_databases() {
  while (start) {
    free_database(start->data);

    struct LinkedListNode *next = start->next;
    free(start);
    start = next;
  }
}
