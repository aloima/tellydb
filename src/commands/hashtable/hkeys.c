#include "../../../headers/telly.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

struct Length {
  uint64_t response;
  uint64_t maximum_line;
};

static struct Length calculate_length(const struct HashTable *table) {
  struct Length length = {
    .response = (3 + (1 + log10(table->size.all))), // array part
    .maximum_line = 0
  };

  for (uint32_t i = 0; i < table->size.allocated; ++i) {
    const struct FVPair *fv = table->fvs[i];

    while (fv) {
      const uint64_t line_length = (5 + (1 + log10(fv->name.len)) + fv->name.len); // name part

      length.maximum_line = fmax(line_length, length.maximum_line);
      length.response += line_length;
      fv = fv->next;
    }
  }

  return length;
}


static void run(struct CommandEntry entry) {
  if (!entry.client) return;
  if (entry.data->arg_count != 1) {
    WRONG_ARGUMENT_ERROR(entry.client, "HKEYS", 5);
    return;
  }

  const struct KVPair *kv = get_data(entry.database, entry.data->args[0]);

  if (!kv) {
    _write(entry.client, "*0\r\n", 4);
  }

  if (kv->type != TELLY_HASHTABLE) {
    _write(entry.client, "-Invalid type for 'HKEYS' command\r\n", 35);
    return;
  }

  const struct HashTable *table = kv->value;
  const struct Length length = calculate_length(table);
  char *response = malloc(length.response + 1);
  char *line = malloc(length.maximum_line + 1);

  sprintf(response, "*%d\r\n", table->size.all);

  for (uint32_t i = 0; i < table->size.allocated; ++i) {
    struct FVPair *fv = table->fvs[i];

    while (fv) {
      sprintf(line, "$%d\r\n%.*s\r\n", fv->name.len, fv->name.len, fv->name.value);
      strcat(response, line);

      fv = fv->next;
    }
  }

  _write(entry.client, response, length.response);
  free(response);
  free(line);
}

const struct Command cmd_hkeys = {
  .name = "HKEYS",
  .summary = "Gets all field names from the hash table.",
  .since = "0.1.9",
  .complexity = "O(N) where N is hash table size",
  .permissions = P_READ,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
