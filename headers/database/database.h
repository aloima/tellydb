#pragma once

#define DATABASE_INITIAL_SIZE 1024

#include "file.h" // IWYU pragma: export
#include "kv.h"   // IWYU pragma: export
#include "list.h" // IWYU pragma: export

#include "../utils/utils.h"

#include <stdint.h>

typedef struct {
  string_t name;
  uint64_t id; // hashed from name

  struct KVPair **data;
  struct {
    uint64_t stored;
    uint64_t capacity;
  } size;
} Database;

Database *create_database(const string_t name, const uint64_t capacity);
LinkedList *get_databases();

void set_main_database(Database *database);
Database *get_main_database();

Database *get_database(const string_t name);
bool rename_database(const string_t old_name, const string_t new_name);
void free_databases();

// TODO: find a way to move them into kv.h
bool delete_kv(Database *database, struct KVPair *kv);
int check_kv_expiry(Database *database, struct KVPair *kv);

struct KVPair *get_data(Database *database, const string_t key);
struct KVPair *set_data(Database *database, struct KVPair *data, const string_t key, void *value, const enum TellyTypes type, const uint64_t *expire_at_p);
bool delete_data(Database *database, const string_t key);
void clear_database(Database *database);
