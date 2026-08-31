#include <telly.h>
#include "file.h"

static inline void generate_integer_value(char **data, off_t *len, const void *value);
static inline void generate_double_value(char **data, off_t *len, const void *value);
static inline void generate_boolean_value(char **data, off_t *len, const void *value);

static inline void generate_string_value_layer(char **data, off_t *len, const void *value) {
  const string_t *string = (const string_t *) value;
  (void) generate_string_value(data, len, string);
}

static inline bool is_primitive(const enum TellyTypes type) {
  switch (type) {
    case TELLY_INT: case TELLY_DOUBLE: case TELLY_STR: case TELLY_BOOL:
      return true;
    default:
      return false;
  }
}

typedef void (*generator_t)(char **data, off_t *len, const void *value);

static const generator_t GENERATORS[] = {
  [TELLY_INT]    = generate_integer_value,
  [TELLY_DOUBLE] = generate_double_value,
  [TELLY_STR]    = generate_string_value_layer,
  [TELLY_BOOL]   = generate_boolean_value
};

typedef struct Buffer {
  char *data;
  off_t *len;
} Buffer;

static inline void generate_integer_value(char **data, off_t *len, const void *value) {
  const mpz_t *number = (const mpz_t *) value;

  const bool negative = (mpz_sgn(*number) == -1);
  const uint8_t byte_count = ((mpz_sizeinbase(*number, 2) + 7) / 8);

  (*data)[*len] = (byte_count - 1) | (negative << 7);
  char *hex = mpz_get_str(NULL, 16, *number);
  const bool is_even = hex[(byte_count * 2) - !negative] != '\0';

  for (uint8_t i = 0; i < byte_count; i++) {
    char byte[3];

    if (i == 0 && !is_even) {
      byte[0] = '0';
      byte[1] = hex[0 + negative];
      byte[2] = '\0';
    } else {
      const uint8_t str_index = ((is_even ? (i * 2) : ((i * 2) - 1)) + negative);
      byte[0] = hex[str_index];
      byte[1] = hex[str_index + 1];
      byte[2] = '\0';
    }

    const uint8_t value = strtol(byte, NULL, 16);
    (*data)[*len += 1] = value;
  }

  *len += 1;
  free(hex);
}

static inline void generate_double_value(char **data, off_t *len, const void *value) {
  const mpf_t *number = (const mpf_t *) value;

  mp_exp_t exp;
  char *hex = mpf_get_str(NULL, &exp, 16, (FLOAT_PRECISION / 8) * 2, *number);
  const uint64_t size = strlen(hex);
  const bool negative = (hex[0] == '-');

  const uint8_t byte_count = (((size - negative) + 1) / 2);
  const bool is_zeroed = (byte_count <= exp);

  (*data)[*len] = ((byte_count - 1) | (negative << 7));
  (*data)[*len += 1] = (!is_zeroed << 7);

  if (!is_zeroed) {
    (*data)[*len] |= exp;
  }

  const bool is_even = (hex[(byte_count * 2) - !negative] != '\0');

  for (uint8_t i = 0; i < byte_count; i++) {
    char byte[3];

    if (i == 0 && !is_even) {
      byte[0] = '0';
      byte[1] = hex[0 + negative];
      byte[2] = '\0';
    } else {
      const uint8_t str_index = ((is_even ? (i * 2) : ((i * 2) - 1)) + negative);
      byte[0] = hex[str_index];
      byte[1] = hex[str_index + 1];
      byte[2] = '\0';
    }

    const uint64_t value = strtol(byte, NULL, 16);
    (*data)[*len += 1] = value;
  }

  *len += 1;
  free(hex);
}

static inline void generate_boolean_value(char **data, off_t *len, const void *value) {
  const bool *boolean = (const bool *) value;

  (*data)[*len] = *boolean;
  *len += 1;
}

static inline void generate_hashtable_element(HashTableElement element, void *external) {
  const HashTableNameValue *field = (HashTableNameValue *) ((void *) &element);
  const Value value = field->value->value;

  Buffer *buffer = (Buffer *) external;
  char *data = buffer->data;
  off_t *len = buffer->len;

  data[*len] = value.type;
  *len += 1;

  (void) generate_string_value(&data, len, field->key);
  GENERATORS[value.type](&data, len, value.data);
}

void generate_headers(char *headers, const uint32_t server_age) {
  // To guarantee endianness
  const typeof(DATABASE_FILE_CONSTANT) magic = _Generic(DATABASE_FILE_CONSTANT,
    uint16_t: htons(DATABASE_FILE_CONSTANT),
    uint32_t: htonl(DATABASE_FILE_CONSTANT)
    // TODO: uint64_t
  );

  ASSERT(memcpy(headers, &magic, sizeof(magic)), !=, NULL);
  ASSERT(memcpy(headers + sizeof(magic), &server_age, sizeof(uint32_t)), !=, NULL);
}

off_t generate_value(char **data, KeyValue *kv) {
  off_t len = 0;
  const enum TellyTypes type = kv->value.type;

  generate_string_value(data, &len, &kv->key);
  (*data)[len++] = type;
  len += 1;

  if (is_primitive(type)) {
    GENERATORS[type](data, &len, kv->value.data);
    return len;
  }

  if (type == TELLY_HASHTABLE) {
      HashTable *table = kv->value.data;
      ASSERT(memcpy(*data + len, &table->size.capacity, sizeof(table->size.capacity)), !=, NULL);
      len += sizeof(table->size.capacity);

      Buffer external = {*data, &len};
      foreach_hashtable(table, generate_hashtable_element, &external);

      (*data)[len] = 0x17;
      len += 1;
  } else if (type == TELLY_LIST) {
    const LinkedList *list = kv->value.data;
    ASSERT(memcpy(*data + len, &list->size, sizeof(list->size)), !=, NULL);
    len += sizeof(list->size);

    const LinkedListNode *node = list->begin;

    while (node) {
      Value *value = (Value *) node->data;
      const enum TellyTypes value_type = value->type;

      (*data)[len] = value_type;
      len += 1;

      if (is_primitive(value_type)) {
        GENERATORS[value_type](data, &len, value->data);
      }

      node = node->next;
    }
  } else {
    return 0;
  }

  return len;
}

uint32_t generate_string_value(char **data, off_t *len, const string_t *string) {
  const uint8_t byte_count = get_byte_count(string->len >> 6);
  const uint8_t first = (byte_count << 6) | (string->len & 0b111111);
  const uint32_t length_in_bytes = string->len >> 6;

  char *_data = *data; 
  off_t _len = *len;

  (_data)[_len] = first;
  _len += 1;

  ASSERT(memcpy(_data + _len, &length_in_bytes, byte_count), !=, NULL);
  _len += byte_count;

  ASSERT(memcpy(_data + _len, string->value, string->len), !=, NULL);
  _len += string->len;

  *data = _data;
  *len = _len;

  return (1 + byte_count + string->len);
}
