#include <telly.h>

bool delete_data(Database *database, string_t key) {
  return delete_from_hashtable(database->data, &key);
}

void clear_database(Database *database) {
  clear_hashtable(database->data, free_hashtablekeyvalue);
}
