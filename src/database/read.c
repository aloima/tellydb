#include <telly.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <sys/types.h>
#include <unistd.h>

#include <gmp.h>

static void collect_bytes(const int fd, char *block, const uint16_t block_size, uint16_t *at, const uint32_t count, void *data) {
  uint32_t remaining = count;

  if ((*at + remaining) >= block_size) {
    const uint16_t available = (block_size - *at);
    memcpy(data + (count - remaining), block + *at, available);
    read(fd, block, block_size);
    remaining -= available;

    while (remaining >= block_size) {
      memcpy(data + (count - remaining), block, block_size);
      read(fd, block, block_size);
      remaining -= block_size;
    }

    memcpy(data + (count - remaining), block, remaining);
    *at = remaining;
  } else {
    memcpy(data, block + *at, remaining);
    *at += remaining;
  }
}

static size_t collect_string(string_t *string, const int fd, char *block, const uint16_t block_size, uint16_t *at) {
  string->len = 0;

  uint8_t first;
  collect_bytes(fd, block, block_size, at, 1, &first);

  const uint8_t byte_count = (first >> 6);
  collect_bytes(fd, block, block_size, at, byte_count, &string->len);

  string->len = ((string->len << 6) | (first & 0b111111));
  string->value = malloc(string->len);
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

static size_t collect_kv(struct KVPair *kv, const int fd, char *block, const uint16_t block_size, uint16_t *at) {
  string_t key;
  void *value = NULL;
  uint8_t type;

  size_t collected_bytes = collect_string(&key, fd, block, block_size, at) + 1;
  collect_bytes(fd, block, block_size, at, 1, &type);

  switch (type) {
    case TELLY_NULL:
      break;

    case TELLY_INT:
      value = malloc(sizeof(mpz_t));
      collected_bytes += collect_integer(value, fd, block, block_size, at);
      break;

    case TELLY_DOUBLE:
      value = malloc(sizeof(mpf_t));
      collected_bytes += collect_double(value, fd, block, block_size, at);
      break;

    case TELLY_STR:
      value = malloc(sizeof(string_t));
      collected_bytes += collect_string(value, fd, block, block_size, at);
      break;

    case TELLY_BOOL:
      value = malloc(sizeof(bool));
      collect_bytes(fd, block, block_size, at, 1, value);
      collected_bytes += 1;
      break;

    case TELLY_HASHTABLE: {
      uint32_t size;
      collect_bytes(fd, block, block_size, at, 4, &size);
      collected_bytes += 5; // includes size bytes and last (0x17) byte

      struct HashTable *table = (value = create_hashtable(size));

      while (true) {
        uint8_t byte;
        collect_bytes(fd, block, block_size, at, 1, &byte);

        if (byte == 0x17) {
          break;
        } else {
          void *fv_value = NULL;
          collected_bytes += 1; // type byte

          string_t name;
          collected_bytes += collect_string(&name, fd, block, block_size, at);

          switch (byte) {
            case TELLY_NULL:
              break;

            case TELLY_INT:
              fv_value = malloc(sizeof(mpz_t));
              collected_bytes += collect_integer(fv_value, fd, block, block_size, at);
              break;

            case TELLY_DOUBLE:
              fv_value = malloc(sizeof(mpf_t));
              collected_bytes += collect_double(fv_value, fd, block, block_size, at);
              break;

            case TELLY_STR:
              fv_value = malloc(sizeof(string_t));
              collected_bytes += collect_string(fv_value, fd, block, block_size, at);
              break;

            case TELLY_BOOL:
              fv_value = malloc(sizeof(bool));
              collect_bytes(fd, block, block_size, at, 1, fv_value);
              collected_bytes += 1;
              break;
          }

          set_field_of_hashtable(table, name, fv_value, byte);
          free(name.value);
        }
      }

      break;
    }

    case TELLY_LIST: {
      uint32_t size;
      collect_bytes(fd, block, block_size, at, 4, &size);
      collected_bytes += (4 + size); // includes size bytes and type bytes of listnodes

      struct List *list = (value = create_list());
      list->size = size;

      for (uint32_t i = 0; i < size; ++i) {
        uint8_t byte;
        void *list_value = NULL;
        collect_bytes(fd, block, block_size, at, 1, &byte);

        switch (byte) {
          case TELLY_NULL:
            break;

          case TELLY_INT:
            list_value = malloc(sizeof(mpz_t));
            collected_bytes += collect_integer(list_value, fd, block, block_size, at);
            break;

          case TELLY_DOUBLE:
            list_value = malloc(sizeof(mpf_t));
            collected_bytes += collect_double(list_value, fd, block, block_size, at);
            break;

          case TELLY_STR:
            list_value = malloc(sizeof(string_t));
            collected_bytes += collect_string(list_value, fd, block, block_size, at);
            break;

          case TELLY_BOOL:
            list_value = malloc(sizeof(bool));
            collect_bytes(fd, block, block_size, at, 1, list_value);
            collected_bytes += 1;
            break;
        }

        struct ListNode *node = create_listnode(list_value, byte);

        node->prev = list->end;
        list->end = node;

        if (i == 0) {
          list->begin = node;
        } else {
          node->prev->next = node;
        }
      }
    }

    break;
  }

  set_kv(kv, key, value, type);
  free(key.value);

  return collected_bytes;
}

static size_t collect_database(struct Database **database, const int fd, char *block, const uint16_t block_size, uint16_t *at, uint64_t *count) {
  collect_bytes(fd, block, block_size, at, 8, count);

  string_t name;
  size_t collected_bytes = collect_string(&name, fd, block, block_size, at) + 8;

  const uint64_t needed = pow(2, get_bit_count(*count));
  const uint64_t capacity = ((needed > DATABASE_INITIAL_SIZE) ? needed : DATABASE_INITIAL_SIZE);

  *database = create_database(name, capacity);
  (*database)->size.stored = *count;
  free(name.value);

  struct KVPair **cache = (*database)->data;

  for (uint32_t i = 0; i < *count; ++i) {
    struct KVPair *kv = malloc(sizeof(struct KVPair));
    collected_bytes += collect_kv(kv, fd, block, block_size, at);

    uint64_t index = (hash(kv->key.value, kv->key.len) % capacity);

    while ((*database)->data[index]) {
      index += 1;
    }

    (*database)->data[index] = kv;
  }

  return collected_bytes;
}

size_t get_all_data_from_file(struct Configuration *conf, const int fd, off_t file_size, char *block, const uint16_t block_size, const uint16_t filled_block_size) {
  size_t loaded_count = 0;
  uint16_t at = filled_block_size;
  const string_t database_name = CREATE_STRING(conf->database_name, strlen(conf->database_name));

  if (at != file_size) {
    const uint64_t hashed = hash(database_name.value, database_name.len);

    off_t collected_bytes = at;
    uint64_t data_count = 0;
    struct Database *database;

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
