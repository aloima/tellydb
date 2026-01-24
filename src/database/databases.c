#include <telly.h>

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

static struct LinkedListNode *head = NULL;
static Database *main = NULL;
static uint32_t database_count = 0;

Database *create_database(const string_t name, const uint64_t capacity) {
  Database *database = NULL;
  struct LinkedListNode *node = NULL;
  char *name_str = NULL;
  struct KVPair **data = NULL;

  database = malloc(sizeof(Database));;
  if (database == NULL) goto CLEANUP;

  node = malloc(sizeof(struct LinkedListNode));
  if (node == NULL) goto CLEANUP;

  name_str = malloc(name.len);
  if (name_str == NULL) goto CLEANUP;

  data = calloc(capacity, sizeof(struct KVPair *));
  if (data == NULL) goto CLEANUP;

  database_count += 1;

  if (database_count != 1) {
    node->data = database;
    node->next = NULL;
  } else {
    head = node;
    node->data = database;
    node->next = NULL;
  }

  database->name = CREATE_STRING(name_str, name.len);
  memcpy(database->name.value, name.value, name.len);

  database->id = hash(name.value, name.len);
  database->data = data;
  database->size.stored = 0;
  database->size.capacity = capacity;

  return database;

CLEANUP:
  if (database) free(database);
  if (node) free(node);
  if (name_str) free(name_str);
  if (data) free(data);

  return NULL;
}

void set_main_database(Database *database) {
  main = database;
}

Database *get_main_database() {
  return main;
}

Database *get_database(const string_t name) {
  const uint64_t target = hash(name.value, name.len);
  struct LinkedListNode *node = head;

  while (node) {
    Database *database = node->data;

    if (database->id == target) {
      return database;
    }

    node = node->next;
  }

  return NULL;
}

struct LinkedListNode *get_database_node() {
  return head;
}

bool rename_database(const string_t old_name, const string_t new_name) {
  const uint64_t target = hash(old_name.value, old_name.len);
  struct LinkedListNode *node = head;

  while (node) {
    Database *database = node->data;

    if (database->id == target) {
      char *name = malloc(new_name.len);
      if (!name) return false;

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

static void free_database(Database *database) {
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
  while (head) {
    free_database(head->data);

    struct LinkedListNode *next = head->next;
    free(head);
    head = next;
  }
}
