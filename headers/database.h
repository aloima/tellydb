// Includes database (file) methods and structures

#pragma once

#include "btree.h"
#include "utils.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <sys/types.h>

/* DATABASE */
struct KVPair {
  string_t key;
  void *value;
  enum TellyTypes type;
};

struct BTree *create_cache();
struct BTree *get_cache();
struct KVPair *get_kv_from_cache(string_t key);
bool delete_kv_from_cache(const char *key, const size_t length);
void free_cache();

void get_all_data_from_file(const int fd, const off64_t file_size, char *block, const uint16_t block_size, const uint16_t filled_block_size);
struct KVPair *get_data(const string_t key);
struct KVPair *set_data(struct KVPair *data, const string_t key, void *value, const enum TellyTypes type);
bool delete_data(const string_t key);
void save_data(const uint64_t server_age);
bool bg_save(uint64_t server_age);

void set_kv(struct KVPair *kv, const string_t key, void *value, const enum TellyTypes type);
bool check_correct_kv(struct KVPair *kv, string_t *key);
void free_kv(struct KVPair *kv);
/* /DATABASE */



/* DATABASE FILE */
bool open_database_fd(const char *filename, uint64_t *server_age);
void close_database_fd();
/* /DATABASE FILE */



/* LISTS */
struct ListNode {
  void *value;
  enum TellyTypes type;

  struct ListNode *prev;
  struct ListNode *next;
};

struct List {
  uint32_t size;
  struct ListNode *begin;
  struct ListNode *end;
};

struct List *create_list();
struct ListNode *create_listnode(void *value, enum TellyTypes type);
void free_listnode(struct ListNode *node);
void free_list(struct List *list);
/* /LISTS */
