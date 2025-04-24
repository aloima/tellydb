#include "../../../headers/telly.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>

struct Length {
  uint64_t response;
  uint64_t maximum_line;
};

static struct Length calculate_length(const enum ProtocolVersion protover, const struct HashTable *table) {
  struct Length length = {
    .response = (3 + (1 + log10(table->size.all * 2))), // array/map part
    .maximum_line = 0
  };

  switch (protover) {
    case RESP2: {
      for (uint32_t i = 0; i < table->size.allocated; ++i) {
        const struct HashTableField *field = table->fields[i];

        while (field) {
          uint64_t line_length = 0;
          line_length += (5 + (1 + log10(field->name.len)) + field->name.len); // name part

          switch (field->type) {
            case TELLY_NULL:
              line_length += 7;
              break;

            case TELLY_NUM:
              line_length += (3 + (1 + log10(*((long *) field->value))));
              break;

            case TELLY_STR: {
              const string_t *string = field->value;
              line_length += (5 + (1 + log10(string->len)) + string->len);;
              break;
            }

            case TELLY_BOOL:
              if (*((bool *) field->value)) line_length += 7;
              else line_length += 8;
              break;

            default: break;
          }

          length.maximum_line = fmax(line_length, length.maximum_line);
          length.response += line_length;
          field = field->next;
        }
      }

      break;
    }

    case RESP3: {
      for (uint32_t i = 0; i < table->size.allocated; ++i) {
        const struct HashTableField *field = table->fields[i];

        while (field) {
          uint64_t line_length = 0;
          line_length += (5 + (1 + log10(field->name.len)) + field->name.len); // name part

          switch (field->type) {
            case TELLY_NULL:
              line_length += 7;
              break;

            case TELLY_NUM:
              line_length += (3 + (1 + log10(*((long *) field->value))));
              break;

            case TELLY_STR: {
              const string_t *string = field->value;
              line_length += (5 + (1 + log10(string->len)) + string->len);;
              break;
            }

            case TELLY_BOOL:
              line_length += 4;
              break;

            default: break;
          }

          length.maximum_line = fmax(line_length, length.maximum_line);
          length.response += line_length;
          field = field->next;
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
    WRONG_ARGUMENT_ERROR(entry.client, "HGETALL", 7);
    return;
  }

  const struct KVPair *kv = get_data(entry.database, entry.data->args[0]);

  if (!kv) {
    switch (entry.client->protover) {
      case RESP2:
        _write(entry.client, "*0\r\n", 4);
        return;

      case RESP3:
        _write(entry.client, "%0\r\n", 4);
        return;
    }
  }

  if (kv->type != TELLY_HASHTABLE) {
    _write(entry.client, "-Invalid type for 'HGETALL' command\r\n", 37);
    return;
  }

  const struct HashTable *table = kv->value;
  const struct Length length = calculate_length(entry.client->protover, table);
  char *response = malloc(length.response + 1);
  char *line = malloc(length.maximum_line + 1);

  switch (entry.client->protover) {
    case RESP2: {
      sprintf(response, "*%" PRIu64 "\r\n", ((uint64_t) table->size.all * 2));

      for (uint32_t i = 0; i < table->size.allocated; ++i) {
        struct HashTableField *field = table->fields[i];

        while (field) {
          sprintf(line, "$%u\r\n%.*s\r\n", field->name.len, field->name.len, field->name.value);
          strcat(response, line);

          switch (field->type) {
            case TELLY_NULL:
              strcpy(line, "+null\r\n");
              break;

            case TELLY_NUM:
              sprintf(line, ":%ld\r\n", *((long *) field->value));
              break;

            case TELLY_STR: {
              const string_t *string = field->value;
              sprintf(line, "$%u\r\n%.*s\r\n", string->len, string->len, string->value);
              break;
            }

            case TELLY_BOOL: {
              if (*((bool *) field->value)) {
                strcpy(line, "+true\r\n");
              } else {
                strcpy(line, "+false\r\n");
              }
            }

            default:
              break;
          }

          strcat(response, line);
          field = field->next;
        }
      }

      break;
    }

    case RESP3: {
      sprintf(response, "%%%u\r\n", table->size.all);

      for (uint32_t i = 0; i < table->size.allocated; ++i) {
        struct HashTableField *field = table->fields[i];

        while (field) {
          sprintf(line, "$%u\r\n%.*s\r\n", field->name.len, field->name.len, field->name.value);
          strcat(response, line);

          switch (field->type) {
            case TELLY_NULL:
              strcpy(line, "+null\r\n");
              break;

            case TELLY_NUM:
              sprintf(line, ":%ld\r\n", *((long *) field->value));
              break;

            case TELLY_STR: {
              const string_t *string = field->value;
              sprintf(line, "$%u\r\n%.*s\r\n", string->len, string->len, string->value);
              break;
            }

            case TELLY_BOOL: {
              if (*((bool *) field->value)) {
                strcpy(line, "#t\r\n");
              } else {
                strcpy(line, "#f\r\n");
              }
            }

            default:
              break;
          }

          strcat(response, line);
          field = field->next;
        }
      }

      break;
    }
  }

  _write(entry.client, response, length.response);
  free(response);
  free(line);
}

const struct Command cmd_hgetall = {
  .name = "HGETALL",
  .summary = "Gets all fields and their values from the hash table.",
  .since = "0.1.9",
  .complexity = "O(N) where N is hash table size",
  .permissions = P_READ,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
