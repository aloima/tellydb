#include <telly.h>

KeyValue *get_data(Database *database, string_t key) {
  HashTableKeyValue *element = (HashTableKeyValue *) get_from_hashtable(database->data, &key);
  if (element == NULL)
    return NULL;

  return element->value;
}
