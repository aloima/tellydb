#include <telly.h>

#include <string.h>
#include <stdint.h>

#include <gmp.h>

static string_t run(struct CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (entry->args->count != 1) {
    return WRONG_ARGUMENT_ERROR("HGETALL");
  }

  const struct KVPair *kv = get_data(entry->database, entry->args->data[0]);

  if (!kv) {
    switch (entry->client->protover) {
      case RESP2:
        return CREATE_STRING("*0\r\n", 4);

      case RESP3:
        return CREATE_STRING("%0\r\n", 4);
    }
  }

  if (kv->type != TELLY_HASHTABLE) {
    return INVALID_TYPE_ERROR("HGETALL");
  }

  const struct HashTable *table = kv->value;
  char *response = entry->client->write_buf;
  uint64_t at;

  switch (entry->client->protover) {
    case RESP2: {
      response[0] = '*';
      at = ltoa(table->size.used * 2, response + 1) + 1;
      response[at++] = '\r';
      response[at++] = '\n';

      for (uint32_t i = 0; i < table->size.capacity; ++i) {
        struct HashTableField *field = table->fields[i];

        if (field) {
          at += create_resp_string(response + at, field->name);

          switch (field->type) {
            case TELLY_NULL:
              memcpy(response + at, "+null\r\n", 7);
              at += 7;
              break;

            case TELLY_INT:
              at += create_resp_integer_mpz(entry->client->protover, response + at, *((mpz_t *) field->value));
              break;

            case TELLY_DOUBLE:
              at += create_resp_integer_mpf(entry->client->protover, response + at, *((mpf_t *) field->value));
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
      response[0] = '%';
      at = ltoa(table->size.used, response + 1) + 1;
      response[at++] = '\r';
      response[at++] = '\n';

      for (uint32_t i = 0; i < table->size.capacity; ++i) {
        struct HashTableField *field = table->fields[i];

        if (field) {
          at += create_resp_string(response + at, field->name);

          switch (field->type) {
            case TELLY_NULL:
              memcpy(response + at, "_\r\n", 3);
              at += 3;
              break;

            case TELLY_INT:
              at += create_resp_integer_mpz(entry->client->protover, response + at, *((mpz_t *) field->value));
              break;

            case TELLY_DOUBLE:
              at += create_resp_integer_mpf(entry->client->protover, response + at, *((mpf_t *) field->value));
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

const struct Command cmd_hgetall = {
  .name = "HGETALL",
  .summary = "Gets all fields and their values from the hash table.",
  .since = "0.1.9",
  .complexity = "O(N) where N is hash table size",
  .permissions = P_READ,
  .flags.value = CMD_FLAG_NO_FLAG,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
