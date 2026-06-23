#include <telly.h>
#include "file.h"

#define GENERATE_PRIMITIVE_VALUES(data, len, value)  \
  case TELLY_NULL:                                   \
    break;                                           \
                                                     \
  case TELLY_INT:                                    \
    generate_integer_value((data), &(len), (value)); \
    break;                                           \
                                                     \
  case TELLY_DOUBLE:                                 \
    generate_double_value((data), &(len), (value));  \
    break;                                           \
                                                     \
  case TELLY_STR:                                    \
    generate_string_value((data), &len, (value));    \
    break;                                           \
                                                     \
  case TELLY_BOOL:                                   \
    generate_boolean_value((data), &len, (value));   \
    break

typedef struct Buffer {
  char *data;
  off_t *len;
} Buffer;

static inline void generate_integer_value(char **data, off_t *len, mpz_t *number) {
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

static inline void generate_double_value(char **data, off_t *len, mpf_t *number) {
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

static inline void generate_boolean_value(char **data, off_t *len, const bool *boolean) {
  (*data)[*len] = *boolean;
  *len += 1;
}

static inline void generate_hashtable_element(HashTableElement element, void *external) {
  const HashTableNameValue *field = (HashTableNameValue *) ((void *) &element);
  const Value value = field->value->value;

  char *data = ((Buffer *) external)->data;
  off_t *len = ((Buffer *) external)->len;

  data[*len] = value.type;
  *len += 1;

  generate_string_value(&data, len, (string_t *) element.key);

  switch (value.type) {
    GENERATE_PRIMITIVE_VALUES(&data, *len, value.data);

    default:
      break;
  }
}

void generate_headers(char *headers, const uint32_t server_age) {
  // To guarantee endianness
  const typeof(DATABASE_FILE_CONSTANT) magic = _Generic(DATABASE_FILE_CONSTANT,
    uint16_t: htons(DATABASE_FILE_CONSTANT),
    uint32_t: htonl(DATABASE_FILE_CONSTANT)
    // TODO: uint64_t
  );

  memcpy(headers, &magic, sizeof(magic));
  memcpy(headers + sizeof(magic), &server_age, sizeof(uint32_t));
}

off_t generate_value(char **data, KeyValue *kv) {
  off_t len = 0;

  generate_string_value(data, &len, &kv->key);
  (*data)[len] = kv->value.type;
  len += 1;

  switch (kv->value.type) {
    GENERATE_PRIMITIVE_VALUES(data, len, kv->value.data);

    case TELLY_HASHTABLE: {
      HashTable *table = kv->value.data;
      memcpy(*data + len, &table->size.capacity, 4);
      len += 4;

      Buffer external = {*data, &len};
      foreach_hashtable(table, generate_hashtable_element, &external);

      (*data)[len] = 0x17;
      len += 1;

      break;
    }

    case TELLY_LIST: {
      const LinkedList *list = kv->value.data;
      memcpy(*data + len, &list->size, 4);
      len += 4;

      const LinkedListNode *node = list->begin;

      while (node) {
        Value *value = (Value *) node->data;
        (*data)[len] = value->type;
        len += 1;

        switch (value->type) {
          GENERATE_PRIMITIVE_VALUES(data, len, value->data);

          default:
            break;
        }

        node = node->next;
      }

      break;
    }

    default:
      len = 0;
      break;
  }

  return len;
}

uint32_t generate_string_value(char **data, off_t *len, const string_t *string) {
  const uint8_t byte_count = get_byte_count(string->len >> 6);
  const uint8_t first = (byte_count << 6) | (string->len & 0b111111);
  const uint32_t length_in_bytes = string->len >> 6;

  (*data)[*len] = first;
  memcpy(*data + (*len += 1), &length_in_bytes, byte_count);
  memcpy(*data + (*len += byte_count), string->value, string->len);
  *len += string->len;

  return (1 + byte_count + string->len);
}
