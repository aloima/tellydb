#include <telly.h>
#include "read.h"
#include "utils/string.h"

static int allocate_value(const GenericArguments *arguments, const UnallocatedValue value, size_t *collected_bytes) {
  const enum TellyTypes type = value.type;
  void **data = value.data;
  uint32_t *element_count = value.element_count;

  if (type == TELLY_LIST || type == TELLY_HASHTABLE) {
    collect_bytes(arguments, 4, element_count);

    if (type == TELLY_LIST) {
      *collected_bytes += (4 + *element_count); // includes size bytes and type bytes of listnodes
      *data = ll_create();
    } else if (type == TELLY_HASHTABLE) {
      *collected_bytes += 5; // includes size bytes and last (0x17) byte
      *data = create_hashtable(*element_count, string_hash, string_compare);
    }
  } else {
    const int64_t size = PRIMARY_TYPE_SIZE_TABLE[type];
    ASSERT(size, !=, -1);
    *data = malloc(size);
  }

  if (*data == NULL)
    return -1;

  return 0;
}

#define COLLECT_PRIMARY_TYPES(_arguments, _value, _collected_bytes, _result) \
  case TELLY_NULL: case TELLY_UNKNOWN:                                       \
    break;                                                                   \
                                                                             \
  case TELLY_INT: {                                                          \
    (_result) = collect_integer((_arguments), (_value));                     \
    if (!(_result).succeed)                                                  \
      return UNSUCCESSFUL_COLLECTION();                                      \
                                                                             \
    (_collected_bytes) += (_result).value;                                   \
    break;                                                                   \
  }                                                                          \
                                                                             \
  case TELLY_DOUBLE: {                                                       \
    (_result) = collect_double((_arguments), (_value));                      \
    if (!(_result).succeed)                                                  \
      return UNSUCCESSFUL_COLLECTION();                                      \
                                                                             \
    (_collected_bytes) += (_result).value;                                   \
    break;                                                                   \
  }                                                                          \
                                                                             \
  case TELLY_STR: {                                                          \
    (_result) = collect_string((_arguments), (_value));                      \
    if (!(_result).succeed)                                                  \
      return UNSUCCESSFUL_COLLECTION();                                      \
                                                                             \
    (_collected_bytes) += (_result).value;                                   \
    break;                                                                   \
  }                                                                          \
                                                                             \
  case TELLY_BOOL: {                                                         \
    collect_bytes((_arguments), 1, (_value));                                \
    (_collected_bytes) += 1;                                                 \
    break;                                                                   \
  }

// TODO: memory checking
static CollectionResult collect_kv(const GenericArguments *arguments, KeyValue *kv) {
  CollectionResult result;

  string_t key;
  void *value = NULL;
  enum TellyTypes type = TELLY_UNKNOWN;

  result = collect_string(arguments, &key);
  if (!result.succeed)
    return UNSUCCESSFUL_COLLECTION();

  size_t collected_bytes = result.value + 1;
  collect_bytes(arguments, 1, (uint8_t *) &type);

  uint32_t element_count = 0;

  const int alloc_ret = ({
    const UnallocatedValue unallocated_value = { .data = &value, .type = type, .element_count = &element_count };
    allocate_value(arguments, unallocated_value, &collected_bytes);
  });

  if (alloc_ret == -1) {
    free(key.value);
    return UNSUCCESSFUL_COLLECTION();
  }

  switch (type) {
    COLLECT_PRIMARY_TYPES(arguments, value, collected_bytes, result);

    case TELLY_HASHTABLE: {
      HashTable *table = (HashTable *) value;

      while (true) {
        uint8_t byte;
        collect_bytes(arguments, 1, &byte);

        if (byte == 0x17)
          break;

        NameValue *field = malloc(sizeof(NameValue));
        if (field == NULL)
          return UNSUCCESSFUL_COLLECTION();

        result = collect_string(arguments, &field->name);
        if (!result.succeed)
          return UNSUCCESSFUL_COLLECTION();

        collected_bytes += result.value;
        field->value.type = byte;
        collected_bytes += 1; // type byte

        void **data = &field->value.data;
        const int field_alloc_ret = ({
          const UnallocatedValue unallocated_value = { .data = data, .type = field->value.type, .element_count = NULL };
          allocate_value(arguments, unallocated_value, &collected_bytes);
        });

        if (field_alloc_ret == -1)
          return UNSUCCESSFUL_COLLECTION();

        switch (field->value.type) {
          COLLECT_PRIMARY_TYPES(arguments, *data, collected_bytes, result);

          default:
            break;
        }

        if (insert_into_hashtable(table, &field->name, field) == NULL)
          return UNSUCCESSFUL_COLLECTION();
      }

      break;
    }

    case TELLY_LIST: {
      LinkedList *list = (LinkedList *) value;

      for (uint32_t i = 0; i < element_count; ++i) {
        uint8_t byte;
        void *list_value = NULL;
        collect_bytes(arguments, 1, &byte);

        const int list_value_alloc_ret = ({
          UnallocatedValue unallocated_value = { .data = &list_value, .type = byte, .element_count = NULL };
          allocate_value(arguments, unallocated_value, &collected_bytes);
        });

        if (list_value_alloc_ret == -1)
          return UNSUCCESSFUL_COLLECTION();

        switch (byte) {
          COLLECT_PRIMARY_TYPES(arguments, list_value, collected_bytes, result);

          default:
            break;
        }

        Value *value = malloc(sizeof(Value));
        if (value == NULL)
          return UNSUCCESSFUL_COLLECTION();

        value->type = byte;
        value->data = list_value;

        if (ll_insert_back(list, value) == NULL)
          return UNSUCCESSFUL_COLLECTION();
      }
    }

    break;
  }

  const int set_kv_ret = set_kv(kv, key, value, type, NULL);
  if (set_kv_ret == -1)
    return UNSUCCESSFUL_COLLECTION();

  free(key.value);
  return CREATE_COLLECTION_RESULT(true, collected_bytes);
}

static CollectionResult collect_database(const GenericArguments *arguments, Database **database, uint64_t *count) {
  KeyValue *kv = NULL;
  *database = NULL;

  collect_bytes(arguments, 8, count);

  string_t name;
  CollectionResult name_result = collect_string(arguments, &name);
  if (!name_result.succeed)
    goto GRACEFUL_SHUTDOWN;

  size_t collected_bytes = name_result.value + 8;

  const uint64_t needed = pow(2, get_bit_count(*count));
  const uint64_t capacity = ((needed > DATABASE_INITIAL_SIZE) ? needed : DATABASE_INITIAL_SIZE);

  *database = create_database(name, capacity);
  free(name.value);

  if (*database == NULL)
    goto GRACEFUL_SHUTDOWN;

  for (uint64_t i = 0; i < *count; ++i) {
    kv = malloc(sizeof(KeyValue));
    if (kv == NULL)
      goto GRACEFUL_SHUTDOWN;

    CollectionResult result = collect_kv(arguments, kv);
    if (!result.succeed)
      goto GRACEFUL_SHUTDOWN;

    collected_bytes += result.value;
    auto insertion = insert_into_hashtable((*database)->data, &kv->key, kv);
    if (insertion == NULL)
      goto GRACEFUL_SHUTDOWN;

    kv = NULL;
  }

  return CREATE_COLLECTION_RESULT(true, collected_bytes);

  GRACEFUL_SHUTDOWN: {
    if (*database != NULL)
      free_database(*database);

    if (kv != NULL)
      free_kv(kv);

    return UNSUCCESSFUL_COLLECTION();
  }
}

off_t read_file(const int fd, const off_t file_size, char *block, const uint16_t block_size, const uint16_t filled_block_size) {
  off_t loaded_count = 0;
  uint16_t at = filled_block_size;
  const string_t database_name = CREATE_STRING(server->conf->database_name, strlen(server->conf->database_name));

  if (at != file_size) {
    const uint64_t hashed = string_hash((string_t *) &database_name);

    off_t collected_bytes = at;
    uint64_t data_count = 0;
    Database *database = NULL;

    const GenericArguments arguments = { .fd = fd, .block = block, .block_size = block_size, .at = &at };

    do {
      CollectionResult result = collect_database(&arguments, &database, &data_count);
      if (!result.succeed)
        return -1;

      collected_bytes += result.value;
      loaded_count += data_count;

      if (database->id == hashed)
        set_main_database(database);
    } while (collected_bytes != file_size);

    if (!get_main_database()) {
      set_main_database(create_database(database_name, DATABASE_INITIAL_SIZE));
    }
  } else {
    set_main_database(create_database(database_name, DATABASE_INITIAL_SIZE));
  }

  return loaded_count;
}
