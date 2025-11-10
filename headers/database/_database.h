#pragma once

#include "_kv.h"
#include "../utils/utils.h"

#include <stdint.h>
#include <stdbool.h>

#include <sys/types.h>

struct DatabaseSize {
  uint64_t stored;
  uint64_t capacity;
};

struct Database {
  string_t name;
  uint64_t id; // hashed from name
  struct KVPair **data;
  struct DatabaseSize size;
  /* struct Client *clients;*/
};

struct Database *create_database(const string_t name, const uint64_t capacity);
struct LinkedListNode *get_database_node();
void set_main_database(struct Database *database);
struct Database *get_main_database();
struct Database *get_database(const string_t name);
bool rename_database(const string_t old_name, const string_t new_name);
void free_databases();

size_t get_all_data_from_file(const int fd, const off_t file_size, char *block, const uint16_t block_size, const uint16_t filled_block_size);
struct KVPair *get_data(struct Database *database, const string_t key);
struct KVPair *set_data(struct Database *database, struct KVPair *data, const string_t key, void *value, const enum TellyTypes type);
bool delete_data(struct Database *database, const string_t key);
