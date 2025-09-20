#include <telly.h>

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

static struct LinkedListNode *start = NULL;
static struct LinkedListNode *end = NULL;
static struct Database *main = NULL;
static uint32_t database_count = 0;

struct Database *create_database(const string_t name, const uint64_t capacity) {
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

  database->name = CREATE_STRING(malloc(name.len), name.len);
  memcpy(database->name.value, name.value, name.len);

  database->id = hash(name.value, name.len);
  database->data = calloc(capacity, sizeof(struct KVPair *));
  database->size.stored = 0;
  database->size.capacity = capacity;

  return database;
}

void set_main_database(struct Database *database) {
  main = database;
}

struct Database *get_main_database() {
  return main;
}

struct Database *get_database(const string_t name) {
  const uint64_t target = hash(name.value, name.len);
  struct LinkedListNode *node = start;

  while (node) {
    struct Database *database = node->data;

    if (database->id == target) {
      return database;
    }

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
      char *name = malloc(new_name.len);

      if (!name) {
        return false;
      }

      database->id = hash(new_name.value, new_name.len);
      free(database->name.value);

      database->name = CREATE_STRING(name, new_name.len);
      memcpy(database->name.value, new_name.value, new_name.len);

      return true;
    }

    node = node->next;
  }

  return false;
}

static void free_database(struct Database *database) {
  for (uint64_t i = 0; i < database->size.capacity; ++i) {
    struct KVPair *kv = database->data[i];

    if (kv) {
      free_kv(kv);
    }
  }

  free(database->data);
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
