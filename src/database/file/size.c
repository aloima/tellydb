#include <telly.h>
#include "file.h"

static inline off_t get_value_size(const enum TellyTypes type, void *value);

static inline void get_hashtable_size(HashTableElement element, void *external) {
  const HashTableNameValue *field = (HashTableNameValue *) ((void *) &element);
  const Value value = field->value->value;
  uint64_t *length = (uint64_t *) external;

  *length += (1 + get_value_size(TELLY_STR, element.key) + get_value_size(value.type, value.data));
}

static inline off_t get_value_size(const enum TellyTypes type, void *value) {
  switch (type) {
    case TELLY_NULL:
      return 0;

    case TELLY_INT: {
      mpz_t *number = value;
      const uint8_t byte_count = ((mpz_sizeinbase(*number, 2) + 7) / 8);

      return (byte_count + 1);
    }

    case TELLY_DOUBLE: {
      mp_exp_t exp;
      mpf_t *number = value;
      char *hex = mpf_get_str(NULL, &exp, 16, 128, *number);
      const bool negative = (hex[0] == '-');

      const uint8_t ret = (strlen(hex) - negative + 2);
      free(hex);

      return ret;
    }

    case TELLY_STR: {
      const string_t *string = (string_t *) value;
      const uint8_t byte_count = get_byte_count(string->len);

      return (byte_count + string->len);
    }

    case TELLY_BOOL:
      return 1;

    case TELLY_HASHTABLE: {
      HashTable *table = (HashTable *) value;
      off_t length = 9;

      foreach_hashtable(table, get_hashtable_size, &length);
      return length;
    }

    case TELLY_LIST: {
      const LinkedList *list = value;
      const LinkedListNode *node = list->begin;
      off_t length = 8;

      while (node) {
        const Value *value = (Value *) node->data;
        length += (1 + get_value_size(value->type, value->data));
        node = node->next;
      }

      return length;
    }

    default:
      return 0;
  }
}

void get_maximum_keyvalue_size(HashTableElement element, void *external) {
  const HashTableKeyValue *kv = (HashTableKeyValue *) ((void *) &element);
  const Value value = kv->value->value;
  uint64_t *max_size = (uint64_t *) external;

  const uint64_t size = (1 + get_value_size(TELLY_STR, (string_t *) element.key) + get_value_size(value.type, value.data));
  *max_size = ((*max_size > size) ? *max_size : size);
}
