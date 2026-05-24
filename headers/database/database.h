#pragma once

#define DATABASE_INITIAL_SIZE 1024

#include "file.h" // IWYU pragma: export
#include "kv.h"   // IWYU pragma: export

#include "../utils/utils.h"

#include <stdint.h>

typedef struct {
  string_t name;
  uint64_t id; // hashed from name
  HashTable *data;
} Database;

uint64_t string_hash(void *data);
bool string_compare(void *string_a, void *string_b);

Database *create_database(const string_t name, const uint64_t capacity);
LinkedList *get_databases();

void set_main_database(Database *database);
Database *get_main_database();

Database *get_database(const string_t name);
bool rename_database(const string_t old_name, const string_t new_name);
void free_databases();

// TODO: find a way to move it into kv.h
int check_kv_expiry(Database *database, KeyValue *kv);

KeyValue *get_data(Database *database, string_t key);
KeyValue *set_data(Database *database, KeyValue *kv, string_t key, void *data, const enum TellyTypes type, const uint64_t *expire_at);
bool delete_data(Database *database, const string_t key);
void clear_database(Database *database);
