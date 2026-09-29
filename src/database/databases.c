#include <telly.h>

static LinkedList *databases = NULL;
static Database *main = NULL;

static inline void free_failed_database(Database *database, char *rname, HashTable *data) {
  free(database);
  free(rname);

  if (data) {
    // There is no data yet, so freeing method is redundant
    destroy_hashtable(data, NULL);
  }
}

#define FREE_FAILED_DATABASE(database, rname, data) do {  \
  free_failed_database(database, rname, data);            \
  return NULL;                                            \
} while (0)

Database *create_database(const string_t name, const uint64_t capacity) {
  Database *database = NULL;
  char *rname = NULL;
  HashTable *data = NULL;

  database = malloc(sizeof(Database));
  if (database == NULL) FREE_FAILED_DATABASE(database, rname, data);

  rname = malloc(name.len);
  if (rname == NULL) FREE_FAILED_DATABASE(database, rname, data);

  data = create_hashtable(capacity, string_hash, string_compare);
  if (data == NULL) FREE_FAILED_DATABASE(database, rname, data);

  if (databases == NULL) {
    databases = ll_create();
    if (databases == NULL) FREE_FAILED_DATABASE(database, rname, data);
  }

  if (ll_insert_back(databases, database) == NULL) FREE_FAILED_DATABASE(database, rname, data);

  database->name = CREATE_STRING(rname, name.len);
  ASSERT(memcpy(database->name.value, name.value, name.len), !=, NULL);

  database->id = string_hash((string_t *) &name);
  database->data = data;

  return database;
}

#undef FREE_FAILED_DATABASE

void set_main_database(Database *database) {
  main = database;
}

Database *get_main_database() {
  return main;
}

LinkedList *get_databases() {
  return databases;
}

typedef struct {
  uint64_t target;
  string_t name;
} ExternalData;

static inline bool cmp(void *data, void *external) {
  Database *database = (Database *) data;
  ExternalData *external_s = ((ExternalData *) external);

  const string_t a = database->name;
  const string_t b = external_s->name;

  return (database->id == external_s->target) && SSTREQ(a, b);
}

Database *get_database(const string_t name) {
  ExternalData external = {
    .name = name,
    .target = string_hash((string_t *) &name)
  };

  LinkedListNode *node = ll_search_node(databases, LL_BACK, &external, cmp);
  return node != NULL ? (Database *) node->data : NULL;
}

bool rename_database(const string_t old_name, const string_t new_name) {
  Database *database = ({
    ExternalData external = {
      .name = old_name,
      .target = string_hash((string_t *) &old_name)
    };

    LinkedListNode *node = ll_search_node(databases, LL_BACK, &external, cmp);
    node != NULL ? (Database *) node->data : NULL;
  });

  if (!database) {
    return false;
  }

  char *name = malloc(new_name.len);
  if (!name) return false;

  database->id = string_hash((string_t *) &new_name);
  free(database->name.value);

  database->name = CREATE_STRING(name, new_name.len);
  ASSERT(memcpy(database->name.value, new_name.value, new_name.len), !=, NULL);

  return true;
}

void free_database(void *database_ptr) {
  Database *database = (Database *) database_ptr;

  destroy_hashtable(database->data, free_hashtablekeyvalue);
  free(database->name.value);
  free(database);
}

void free_databases() {
  ll_free(databases, free_database);
}
