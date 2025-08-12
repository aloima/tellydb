#include <telly.h>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

static const uint64_t calculate_length(const enum ProtocolVersion protover, const struct HashTable *table) {
  uint64_t length = (3 + get_digit_count(table->size.used)); // *number\r\n

  switch(protover) {
    case RESP2: {
      for (uint32_t i = 0; i < table->size.capacity; ++i) {
        const struct HashTableField *field = table->fields[i];
    
        if (field) {
          switch (field->type) {
            case TELLY_NULL:
              length += 7; // +null\r\n
              break;
    
            case TELLY_NUM:
              length += (3 + get_digit_count(*((long *) field->value))); // :number\r\n
              break;
    
            case TELLY_STR: {
              const string_t *string = field->value;
              length += (5 + get_digit_count(string->len) + string->len); // $length\r\nstring\r\n
              break;
            }
    
            case TELLY_BOOL:
              if (*((bool *) field->value)) {
                length += 7; // +true\r\n
              } else {
                length += 8; // +false\r\n
              }

              break;
    
            default:
              break;
          }
        }
      }

      break;
    }

    case RESP3: {
      for (uint32_t i = 0; i < table->size.capacity; ++i) {
        const struct HashTableField *field = table->fields[i];
    
        if (field) {
          switch (field->type) {
            case TELLY_NULL:
              length += 3; // _\r\n
              break;
    
            case TELLY_NUM:
              length += (3 + get_digit_count(*((long *) field->value))); // :number\r\n
              break;
    
            case TELLY_STR: {
              const string_t *string = field->value;
              length += (5 + get_digit_count(string->len) + string->len); // $length\r\nstring\r\n
              break;
            }
    
            case TELLY_BOOL:
              length += 4; // #t\r\n or #f\r\n
              break;
    
            default:
              break;
          }
        }
      }

      break;
    }
  }

  return length;
}


static void run(struct CommandEntry entry) {
  if (!entry.client) return;
  if (entry.data->arg_count != 1) {
    WRONG_ARGUMENT_ERROR(entry.client, "HVALS");
    return;
  }

  const struct KVPair *kv = get_data(entry.database, entry.data->args[0]);

  if (!kv) {
    _write(entry.client, "*0\r\n", 4);
    return;
  }

  if (kv->type != TELLY_HASHTABLE) {
    INVALID_TYPE_ERROR(entry.client, "HVALS");
    return;
  }

  const struct HashTable *table = kv->value;
  const uint64_t length = calculate_length(entry.client->protover, table);
  char *response = malloc(length);

  response[0] = '*';
  uint64_t at = ltoa(table->size.used, response + 1) + 1;
  response[at++] = '\r';
  response[at++] = '\n';

  switch (entry.client->protover) {
    case RESP2: {
      for (uint32_t i = 0; i < table->size.capacity; ++i) {
        struct HashTableField *field = table->fields[i];

        if (field) {
          switch (field->type) {
            case TELLY_NULL:
              memcpy(response + at, "+null\r\n", 7);
              at += 7;
              break;

            case TELLY_NUM:
              at += create_resp_integer(response + at, *((long *) field->value));
              break;

            case TELLY_STR:
              at += create_resp_string(response + at, *((string_t *) field->value));
              break;

            case TELLY_BOOL: {
              if (*((bool *) field->value)) {
                memcpy(response + at, "+true\r\n", 7);
                at += 7;
              } else {
                memcpy(response + at, "+false\r\n", 8);
                at += 8;
              }
            }

            default:
              break;
          }
        }
      }

      break;
    }

    case RESP3: {
      for (uint32_t i = 0; i < table->size.capacity; ++i) {
        struct HashTableField *field = table->fields[i];

        if (field) {
          switch (field->type) {
            case TELLY_NULL:
              memcpy(response + at, "_\r\n", 3);
              at += 3;
              break;

            case TELLY_NUM:
              at += create_resp_integer(response + at, *((long *) field->value));
              break;

            case TELLY_STR: {
              at += create_resp_string(response + at, *((string_t *) field->value));
              break;
            }

            case TELLY_BOOL: {
              if (*((bool *) field->value)) {
                memcpy(response + at, "#t\r\n", 4);
              } else {
                memcpy(response + at, "#f\r\n", 4);
              }

              at += 4;
            }

            default:
              break;
          }
        }
      }

      break;
    }
  }

  _write(entry.client, response, length);
  free(response);
}

const struct Command cmd_hvals = {
  .name = "HVALS",
  .summary = "Gets all field values from the hash table.",
  .since = "0.1.9",
  .complexity = "O(N) where N is hash table size",
  .permissions = P_READ,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
