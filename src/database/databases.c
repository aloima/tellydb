#include <telly.h>

static inline Database *allocate_database(const string_t name, const uint64_t capacity) {
  Database *database = NULL;
  char *rname = NULL;

  database = malloc(sizeof(Database));
  if (database == NULL) return NULL;

  rname = malloc(name.len);

  if (rname == NULL) {
    free(database);
    return NULL;
  }

  database->id = string_hash((string_t *) &name);
  database->name = CREATE_STRING(rname, name.len);
  ASSERT(memcpy(database->name.value, name.value, name.len), !=, NULL);

  database->data = create_hashtable(capacity, string_hash, string_compare);

  if (database->data == NULL) {
    free(database);
    free(rname);
    return NULL;
  }

  return database;
}

Database *create_database(const string_t name, const uint64_t capacity) {
  Database *database = allocate_database(name, capacity);
  if (database == NULL) return NULL;

  if (server->databases == NULL) {
    server->databases = ll_create();

    if (server->databases == NULL) {
      free_database(database);
      return NULL;
    }
  }

  if (ll_insert_back(server->databases, database) == NULL) {
    free_database(database);
    return NULL;
  }

  return database;
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

  LinkedListNode *node = ll_search_node(server->databases, LL_BACK, &external, cmp);
  return node != NULL ? (Database *) node->data : NULL;
}

bool rename_database(const string_t old_name, const string_t new_name) {
  Database *database = get_database(old_name);
  if (!database) return false;

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
  ll_free(server->databases, free_database);
}
