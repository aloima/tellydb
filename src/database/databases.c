#include <telly.h>

static LinkedList *databases = NULL;
static Database *main = NULL;

Database *create_database(const string_t name, const uint64_t capacity) {
  Database *database = NULL;
  char *name_str = NULL;
  struct KVPair **data = NULL;

  database = malloc(sizeof(Database));
  if (database == NULL) goto CLEANUP;

  name_str = malloc(name.len + 1);
  if (name_str == NULL) goto CLEANUP;

  data = calloc(capacity, sizeof(struct KVPair *));
  if (data == NULL) goto CLEANUP;

  if (databases == NULL) {
    databases = ll_create();
    if (databases == NULL) goto CLEANUP;
  }

  if (ll_insert_back(databases, database) == NULL)
    goto CLEANUP;

  database->name = CREATE_STRING(name_str, name.len);
  memcpy(database->name.value, name.value, name.len);

  database->id = OPENSSL_LH_strhash(name.value);
  database->data = data;
  database->size.stored = 0;
  database->size.capacity = capacity;

  return database;

CLEANUP:
  if (database) free(database);
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

LinkedList *get_databases() {
  return databases;
}

struct ExternalData {
  uint64_t target;
  string_t name;
};

static inline bool cmp(void *data, void *external) {
  Database *database = (Database *) data;
  struct ExternalData *external_s = ((struct ExternalData *) external);
  return (database->id == external_s->target) &&
            (database->name.len == external_s->name.len && memcmp(database->name.value, external_s->name.value, 0) == 0);
}

Database *get_database(const string_t name) {
  struct ExternalData external = {
    .name = name,
    .target = OPENSSL_LH_strhash(name.value)
  };

  LinkedListNode *node = ll_search_node(databases, LL_BACK, &external, cmp);
  return node != NULL ? (Database *) node->data : NULL;
}

bool rename_database(const string_t old_name, const string_t new_name) {
  Database *database = ({
    struct ExternalData external = {
      .name = old_name,
      .target = OPENSSL_LH_strhash(old_name.value)
    };

    LinkedListNode *node = ll_search_node(databases, LL_BACK, &external, cmp);
    node != NULL ? (Database *) node->data : NULL;
  });

  if (!database) return false;

  char *name = malloc(new_name.len);
  if (!name) return false;

  database->id = OPENSSL_LH_strhash(new_name.value);
  free(database->name.value);

  database->name = CREATE_STRING(name, new_name.len);
  memcpy(database->name.value, new_name.value, new_name.len);

  return true;
}

static void free_database(void *database_ptr) {
  Database *database = (Database *) database_ptr;

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
  ll_free(databases, free_database);
}
