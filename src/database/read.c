#include <telly.h>

// Will be used for method arguments, includes everything which will be used except individual data such as string
typedef struct GenericArguments {
  const int fd;
  char *block;
  const uint16_t block_size;
  uint16_t *at;
} GenericArguments;

typedef struct UnallocatedValue {
  void **data;
  const enum TellyTypes type;
  uint32_t *element_count;
} UnallocatedValue;

static void collect_bytes(const GenericArguments *arguments, const uint32_t count, void *data) {
  auto fd = arguments->fd;
  auto block = arguments->block;
  auto block_size = arguments->block_size;
  auto at = arguments->at;

  uint32_t remaining = count;

  if ((*at + remaining) < block_size) {
    ASSERT(memcpy(data, block + *at, remaining), !=, NULL);
    *at += remaining;
    return;
  }

  const uint16_t available = (block_size - *at);
  ASSERT(memcpy(data + (count - remaining), block + *at, available), !=, NULL);
  ASSERT(read(fd, block, block_size), ==, (int64_t) block_size);
  remaining -= available;

  while (remaining >= block_size) {
    ASSERT(memcpy(data + (count - remaining), block, block_size), !=, NULL);
    ASSERT(read(fd, block, block_size), ==, (int64_t) block_size);
    remaining -= block_size;
  }

  ASSERT(memcpy(data + (count - remaining), block, remaining), !=, NULL);
  *at = remaining;
}

static size_t collect_string(const GenericArguments *arguments, string_t *string) {
  string->len = 0;

  uint8_t first;
  collect_bytes(arguments, 1, &first);

  const uint8_t byte_count = (first >> 6);
  collect_bytes(arguments, byte_count, &string->len);

  string->len = ((string->len << 6) | (first & 0b111111));
  string->value = malloc(string->len);
  collect_bytes(arguments, string->len, string->value);

  return (1 + byte_count + string->len);
}

static size_t collect_integer(const GenericArguments *arguments, mpz_t *number) {
  uint8_t specifier;
  collect_bytes(arguments, 1, &specifier);

  const bool negative = (specifier & 0x80);
  const uint8_t byte_count = ((specifier & 0x7F) + 1);

  uint8_t data[byte_count];
  collect_bytes(arguments, byte_count, data);

  mpz_t value;
  mpz_init(value);
  mpz_init_set_ui(*number, 0);

  const uint8_t end = (byte_count - 1);

  for (uint8_t i = 0; i < end; ++i) {
    mpz_set_ui(value, data[i]);
    mpz_ior(*number, *number, value);
    mpz_mul_2exp(*number, *number, 8);
  }

  mpz_set_ui(value, data[end]);
  mpz_ior(*number, *number, value);

  if (negative) {
    mpz_neg(*number, *number);
  }

  mpz_clear(value);
  return (1 + byte_count);
}

static size_t collect_double(const GenericArguments *arguments, mpf_t *number) {
  uint8_t two[2];
  collect_bytes(arguments, 2, two);

  uint8_t specifier = two[0], indicator = two[1];
  const bool negative = (specifier & 0x80);
  const uint8_t byte_count = ((specifier & 0x7F) + 1);
  int16_t exp = ((indicator & 0x80) ? (indicator & 0x7F) : -1);

  uint8_t data[byte_count];
  collect_bytes(arguments, byte_count, data);

  const bool is_even = (data[0] > 0x10);

  const uint16_t hex_len = ((byte_count * 2) + (exp != -1));
  char hex[hex_len + 1];

  if (!is_even) {
    exp += 1;
  }

  if (exp != -1) {
    const bool exp_even = ((exp & 1) == 0); // (exp % 2) == 0

    if (exp_even) {
      for (uint8_t i = 0; i < byte_count; ++i) {
        if ((i * 2) == exp) {
          sprintf(hex + (i * 2), ".%02x", data[i]);
        } else {
          sprintf(hex + (i * 2) + (exp < (i * 2)), "%02x", data[i]);
        }
      }
    } else {
      for (uint8_t i = 0; i < byte_count; ++i) {
        const uint8_t mul = (i * 2);

        if ((mul + 1) == exp) {
          char value[3];
          sprintf(value, "%02x", data[i]);

          hex[mul] = value[0];
          hex[mul + 1] = '.';
          hex[mul + 2] = value[1];
        } else {
          sprintf(hex + (i * 2) + (exp < (mul + 1)), "%02x", data[i]);
        }
      }
    }
  } else {
    for (uint8_t i = 0; i < byte_count; ++i) {
      sprintf(hex + (i * 2), "%02x", data[i]);
    }
  }

  hex[hex_len] = '\0';

  mpf_init2(*number, FLOAT_PRECISION);
  mpf_set_str(*number, hex, 16);

  if (negative) {
    mpf_neg(*number, *number);
  }

  return (2 + byte_count);
}

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

// TODO: memory checking
static size_t collect_kv(const GenericArguments *arguments, KeyValue *kv) {
  string_t key;
  void *value = NULL;
  enum TellyTypes type = TELLY_UNKNOWN;

  size_t collected_bytes = collect_string(arguments, &key) + 1;
  collect_bytes(arguments, 1, (uint8_t *) &type);

  uint32_t element_count = 0;

  const int alloc_ret = ({
    const UnallocatedValue unallocated_value = { .data = &value, .type = type, .element_count = &element_count };
    allocate_value(arguments, unallocated_value, &collected_bytes);
  });

  if (alloc_ret == -1) {
    // TODO
  }

  switch (type) {
    case TELLY_NULL: case TELLY_UNKNOWN: break;
  
    case TELLY_INT:
      collected_bytes += collect_integer(arguments, value);
      break;

    case TELLY_DOUBLE:
      collected_bytes += collect_double(arguments, value);
      break;

    case TELLY_STR:
      collected_bytes += collect_string(arguments, value);
      break;

    case TELLY_BOOL:
      collect_bytes(arguments, 1, value);
      collected_bytes += 1;
      break;

    case TELLY_HASHTABLE: {
      HashTable *table = (HashTable *) value;

      while (true) {
        uint8_t byte;
        collect_bytes(arguments, 1, &byte);

        if (byte == 0x17) {
          break;
        }

        NameValue *field = malloc(sizeof(NameValue));
        collected_bytes += collect_string(arguments, &field->name);
        field->value.type = byte;
        collected_bytes += 1; // type byte

        void **data = &field->value.data;
        const int field_alloc_ret = ({
          const UnallocatedValue unallocated_value = { .data = data, .type = field->value.type, .element_count = NULL };
          allocate_value(arguments, unallocated_value, &collected_bytes);
        });

        if (field_alloc_ret == -1) {
          // TODO
        }

        switch (field->value.type) {
          case TELLY_INT:
            collected_bytes += collect_integer(arguments, *data);
            break;

          case TELLY_DOUBLE:
            collected_bytes += collect_double(arguments, *data);
            break;

          case TELLY_STR:
            collected_bytes += collect_string(arguments, *data);
            break;

          case TELLY_BOOL:
            collect_bytes(arguments, 1, *data);
            collected_bytes += 1;
            break;

          default:
            break;
        }

        insert_into_hashtable(table, &field->name, field);
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

        if (list_value_alloc_ret == -1) {
          // TODO
        }

        switch (byte) {
          case TELLY_NULL:
            break;

          case TELLY_INT:
            collected_bytes += collect_integer(arguments, list_value);
            break;

          case TELLY_DOUBLE:
            collected_bytes += collect_double(arguments, list_value);
            break;

          case TELLY_STR:
            collected_bytes += collect_string(arguments, list_value);
            break;

          case TELLY_BOOL:
            collect_bytes(arguments, 1, list_value);
            collected_bytes += 1;
            break;

          default:
            break;
        }

        Value *value = malloc(sizeof(Value));
        value->type = byte;
        value->data = list_value;

        ll_insert_back(list, value);
      }
    }

    break;
  }

  set_kv(kv, key, value, type, NULL);
  free(key.value);

  return collected_bytes;
}

static size_t collect_database(const GenericArguments *arguments, Database **database, uint64_t *count) {
  collect_bytes(arguments, 8, count);

  string_t name;
  size_t collected_bytes = collect_string(arguments, &name) + 8;

  const uint64_t needed = pow(2, get_bit_count(*count));
  const uint64_t capacity = ((needed > DATABASE_INITIAL_SIZE) ? needed : DATABASE_INITIAL_SIZE);

  *database = create_database(name, capacity);
  free(name.value);

  for (uint64_t i = 0; i < *count; ++i) {
    KeyValue *kv = malloc(sizeof(KeyValue));
    collected_bytes += collect_kv(arguments, kv);

    (void) insert_into_hashtable((*database)->data, &kv->key, kv);
  }

  return collected_bytes;
}

size_t read_file(const int fd, const off_t file_size, char *block, const uint16_t block_size, const uint16_t filled_block_size) {
  size_t loaded_count = 0;
  uint16_t at = filled_block_size;
  const string_t database_name = CREATE_STRING(server->conf->database_name, strlen(server->conf->database_name));

  if (at != file_size) {
    const uint64_t hashed = string_hash((string_t *) &database_name);

    off_t collected_bytes = at;
    uint64_t data_count = 0;
    Database *database = NULL;

    const GenericArguments arguments = { .fd = fd, .block = block, .block_size = block_size, .at = &at };

    do {
      collected_bytes += collect_database(&arguments, &database, &data_count);
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
