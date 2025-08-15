#pragma once

#include "_kv.h"
#include "../config.h"
#include "../utils.h"
#include "../btree.h"

#include <stdint.h>
#include <stdbool.h>

#include <sys/types.h>

struct Database {
  string_t name;
  uint64_t id; // hashed from name
  struct BTree *cache;
  /* struct Client *clients;*/
};

struct Database *create_database(const string_t name);
struct LinkedListNode *get_database_node();
void set_main_database(struct Database *database);
struct Database *get_main_database();
struct BTree *get_cache_of_database(const string_t name);
bool rename_database(const string_t old_name, const string_t new_name);
void free_databases();

size_t get_all_data_from_file(struct Configuration *conf, const int fd, const off_t file_size, char *block, const uint16_t block_size, const uint16_t filled_block_size);
struct KVPair *get_data(struct Database *database, const string_t key);
struct KVPair *set_data(struct Database *database, struct KVPair *data, const string_t key, void *value, const enum TellyTypes type);
bool delete_data(struct Database *database, const string_t key);
void save_data(const uint32_t server_age);
bool bg_save(const uint32_t server_age);
