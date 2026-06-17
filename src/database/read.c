#include <telly.h>

static void collect_bytes(const int fd, char *block, const uint16_t block_size, uint16_t *at, const uint32_t count, void *data) {
  uint32_t remaining = count;

  if ((*at + remaining) >= block_size) {
    const uint16_t available = (block_size - *at);
    memcpy(data + (count - remaining), block + *at, available);
    ASSERT(read(fd, block, block_size), ==, (int64_t) block_size);
    remaining -= available;

    while (remaining >= block_size) {
      memcpy(data + (count - remaining), block, block_size);
      ASSERT(read(fd, block, block_size), ==, (int64_t) block_size);
      remaining -= block_size;
    }

    memcpy(data + (count - remaining), block, remaining);
    *at = remaining;
  } else {
    memcpy(data, block + *at, remaining);
    *at += remaining;
  }
}

static size_t collect_string(string_t *string, const int fd, char *block, const uint16_t block_size, uint16_t *at, bool nt) {
  string->len = 0;

  uint8_t first;
  collect_bytes(fd, block, block_size, at, 1, &first);

  const uint8_t byte_count = (first >> 6);
  collect_bytes(fd, block, block_size, at, byte_count, &string->len);

  string->len = ((string->len << 6) | (first & 0b111111));

  if (nt) {
    string->value = malloc(string->len + 1);
    string->value[string->len] = '\0';
  } else {
    string->value = malloc(string->len);
  }

  collect_bytes(fd, block, block_size, at, string->len, string->value);

  return (1 + byte_count + string->len);
}

static size_t collect_integer(mpz_t *number, const int fd, char *block, const uint16_t block_size, uint16_t *at) {
  uint8_t specifier;
  collect_bytes(fd, block, block_size, at, 1, &specifier);

  const bool negative = (specifier & 0x80);
  const uint8_t byte_count = ((specifier & 0x7F) + 1);

  uint8_t data[byte_count];
  collect_bytes(fd, block, block_size, at, byte_count, data);

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

static size_t collect_double(mpf_t *number, const int fd, char *block, const uint16_t block_size, uint16_t *at) {
  uint8_t two[2];
  collect_bytes(fd, block, block_size, at, 2, two);

  uint8_t specifier = two[0], indicator = two[1];
  const bool negative = (specifier & 0x80);
  const uint8_t byte_count = ((specifier & 0x7F) + 1);
  int16_t exp = ((indicator & 0x80) ? (indicator & 0x7F) : -1);

  uint8_t data[byte_count];
  collect_bytes(fd, block, block_size, at, byte_count, data);

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

typedef struct AllocationArguments {
  const int fd;
  char *block;
  const uint16_t block_size;
  uint16_t *at;
  size_t *collected_bytes;
  uint32_t *element_count;
} AllocationArguments;

static inline AllocationArguments create_allocation_arguments(
  const int fd, char *block, const uint16_t block_size, uint16_t *at, size_t *collected_bytes, uint32_t *element_count
) {
  AllocationArguments arguments = {
    .fd = fd,
    .at = at,
    .block = block,
    .block_size = block_size,
    .collected_bytes = collected_bytes,
    .element_count = element_count,
  };

  return arguments;
}

static int allocate_value(void **value, const enum TellyTypes type, AllocationArguments arguments) {
  if (type == TELLY_LIST || type == TELLY_HASHTABLE) {
    collect_bytes(arguments.fd, arguments.block, arguments.block_size, arguments.at, 4, arguments.element_count);

    if (type == TELLY_LIST) {
      *(arguments.collected_bytes) += (4 + *(arguments.element_count)); // includes size bytes and type bytes of listnodes
      *value = ll_create();
    } else if (type == TELLY_HASHTABLE) {
      *(arguments.collected_bytes) += 5; // includes size bytes and last (0x17) byte
      *value = create_hashtable(*(arguments.element_count), string_hash, string_compare);
    }
  } else {
    const int64_t size = PRIMARY_TYPE_SIZE_TABLE[type];
    ASSERT(size, !=, -1);
    *value = malloc(size);
  }

  if (*value == NULL)
    return -1;

  return 0;
}

// TODO: memory checking
static size_t collect_kv(KeyValue *kv, const int fd, char *block, const uint16_t block_size, uint16_t *at) {
  string_t key;
  void *value = NULL;
  uint8_t type;

  size_t collected_bytes = collect_string(&key, fd, block, block_size, at, false) + 1;
  collect_bytes(fd, block, block_size, at, 1, &type);

  const int64_t size = PRIMARY_TYPE_SIZE_TABLE[type];
  uint32_t element_count;

  AllocationArguments arguments = create_allocation_arguments(fd, block, block_size, at, &collected_bytes, &element_count);
  const int allocated = allocate_value(&value, type, arguments);

  if (allocated == -1) {
    // TODO
  }

  switch (type) {
    case TELLY_INT:
      collected_bytes += collect_integer(value, fd, block, block_size, at);
      break;

    case TELLY_DOUBLE:
      collected_bytes += collect_double(value, fd, block, block_size, at);
      break;

    case TELLY_STR:
      collected_bytes += collect_string(value, fd, block, block_size, at, false);
      break;

    case TELLY_BOOL:
      collect_bytes(fd, block, block_size, at, 1, value);
      collected_bytes += 1;
      break;

    case TELLY_HASHTABLE: {
      HashTable *table = (HashTable *) value;

      while (true) {
        uint8_t byte;
        collect_bytes(fd, block, block_size, at, 1, &byte);

        if (byte == 0x17) {
          break;
        }

        NameValue *field = malloc(sizeof(NameValue));
        collected_bytes += collect_string(&field->name, fd, block, block_size, at, true);
        field->value.type = byte;
        collected_bytes += 1; // type byte

        void **data = &field->value.data;

        AllocationArguments field_arguments = create_allocation_arguments(fd, block, block_size, at, &collected_bytes, NULL);
        const int allocated = allocate_value(data, field->value.type, field_arguments);

        if (allocated == -1) {
          // TODO
        }

        switch (field->value.type) {
          case TELLY_INT:
            collected_bytes += collect_integer(*data, fd, block, block_size, at);
            break;

          case TELLY_DOUBLE:
            collected_bytes += collect_double(*data, fd, block, block_size, at);
            break;

          case TELLY_STR:
            collected_bytes += collect_string(*data, fd, block, block_size, at, false);
            break;

          case TELLY_BOOL:
            collect_bytes(fd, block, block_size, at, 1, *data);
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

      for (uint32_t i = 0; i < size; ++i) {
        uint8_t byte;
        void *list_value = NULL;
        collect_bytes(fd, block, block_size, at, 1, &byte);

        AllocationArguments list_value_arguments = create_allocation_arguments(fd, block, block_size, at, &collected_bytes, NULL);
        const int allocated = allocate_value(&list_value, byte, list_value_arguments);

        if (allocated == -1) {
          // TODO
        }

        switch ((const enum TellyTypes) byte) {
          case TELLY_NULL:
            break;

          case TELLY_INT:
            collected_bytes += collect_integer(list_value, fd, block, block_size, at);
            break;

          case TELLY_DOUBLE:
            collected_bytes += collect_double(list_value, fd, block, block_size, at);
            break;

          case TELLY_STR:
            collected_bytes += collect_string(list_value, fd, block, block_size, at, false);
            break;

          case TELLY_BOOL:
            collect_bytes(fd, block, block_size, at, 1, list_value);
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

static size_t collect_database(Database **database, const int fd, char *block, const uint16_t block_size, uint16_t *at, uint64_t *count) {
  collect_bytes(fd, block, block_size, at, 8, count);

  string_t name;
  size_t collected_bytes = collect_string(&name, fd, block, block_size, at, true) + 8;

  const uint64_t needed = pow(2, get_bit_count(*count));
  const uint64_t capacity = ((needed > DATABASE_INITIAL_SIZE) ? needed : DATABASE_INITIAL_SIZE);

  *database = create_database(name, capacity);
  free(name.value);

  for (uint64_t i = 0; i < *count; ++i) {
    KeyValue *kv = malloc(sizeof(KeyValue));
    collected_bytes += collect_kv(kv, fd, block, block_size, at);

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
    Database *database;

    do {
      collected_bytes += collect_database(&database, fd, block, block_size, &at, &data_count);
      loaded_count += data_count;

      if (database->id == hashed) {
        set_main_database(database);
      }
    } while (collected_bytes != file_size);

    if (!get_main_database()) {
      set_main_database(create_database(database_name, DATABASE_INITIAL_SIZE));
    }
  } else {
    set_main_database(create_database(database_name, DATABASE_INITIAL_SIZE));
  }

  return loaded_count;
}
