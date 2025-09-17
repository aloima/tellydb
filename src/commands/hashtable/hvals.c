#include <telly.h>

#include <string.h>
#include <stdint.h>

#include <gmp.h>

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

            case TELLY_INT: {
              // This way may return an additional byte because of gmp logic
              mpz_t *value = field->value;
              length += (3 + mpz_sizeinbase(*value, 10)); // :number\r\n

              if (mpz_sgn(*value) == -1) {
                length += 1;
              }

              break;
            }

            case TELLY_DOUBLE: {
              // TODO: there is no way to get length of float without allocation
              // even if allocation will be done, it may not be a proper way,
              // because create_resp_integer_mpf logic is different from gmp
              length += 1;
              break;
            }
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

            case TELLY_INT: {
              // This way may return an additional byte because of gmp logic
              mpz_t *value = field->value;
              length += (3 + mpz_sizeinbase(*value, 10)); // :number\r\n

              if (mpz_sgn(*value) == -1) {
                length += 1;
              }

              break;
            }

            case TELLY_DOUBLE: {
              // TODO: there is no way to get length of float without allocation
              // even if allocation will be done, it may not be a proper way,
              // because create_resp_integer_mpf logic is different from gmp
              length += 1;
              break;
            }

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


static string_t run(struct CommandEntry entry) {
  PASS_NO_CLIENT(entry.client);

  if (entry.data->arg_count != 1) {
    return WRONG_ARGUMENT_ERROR("HVALS");
  }

  const struct KVPair *kv = get_data(entry.database, entry.data->args[0]);

  if (!kv) {
    return CREATE_STRING("*0\r\n", 4);
  }

  if (kv->type != TELLY_HASHTABLE) {
    return INVALID_TYPE_ERROR("HVALS");
  }

  const struct HashTable *table = kv->value;
  const uint64_t length = calculate_length(entry.client->protover, table);
  char *response = entry.buffer;

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

            case TELLY_INT:
              at += create_resp_integer_mpz(entry.client->protover, response + at, *((mpz_t *) field->value));
              break;

            case TELLY_DOUBLE:
              at += create_resp_integer_mpf(entry.client->protover, response + at, *((mpf_t *) field->value));
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

            case TELLY_INT:
              at += create_resp_integer_mpz(entry.client->protover, response + at, *((mpz_t *) field->value));
              break;

            case TELLY_DOUBLE:
              at += create_resp_integer_mpf(entry.client->protover, response + at, *((mpf_t *) field->value));
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

  return CREATE_STRING(response, at);
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
