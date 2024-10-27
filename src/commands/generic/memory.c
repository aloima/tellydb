#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/hashtable.h"
#include "../../../headers/utils.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

static uint32_t get_value_size(void *value, const enum TellyTypes type) {
  switch (type) {
    case TELLY_NULL:
      return 0;

    case TELLY_NUM:
      return sizeof(long);

    case TELLY_STR:
      return sizeof(string_t) + ((((string_t *) value)->len) * sizeof(char));

    case TELLY_BOOL:
      return sizeof(bool);

    case TELLY_HASHTABLE: {
      struct HashTable *table = value;
      uint32_t size = sizeof(struct HashTable);

      for (uint32_t i = 0; i < table->size.allocated; ++i) {
        struct FVPair *fv = table->fvs[i];

        while (fv) {
          size += sizeof(struct FVPair *) + sizeof(struct FVPair) + ((fv->name.len + 1) * sizeof(char)) + get_value_size(fv->value, fv->type);
          fv = fv->next;
        }
      }

      return size;
    }

    case TELLY_LIST: {
      struct List *list = value;
      uint32_t size = sizeof(struct List);
      struct ListNode *node = list->begin;

      while (node) {
        size += get_value_size(node->value, node->type);
        node = node->next;
      }

      return size;
    }
  }

  return 0;
}

static void run(struct Client *client, commanddata_t *command) {
  if (client) {
    if (command->arg_count != 0) {
      const string_t subcommand_string = command->args[0];
      char subcommand[subcommand_string.len + 1];
      to_uppercase(subcommand_string.value, subcommand);

      if (streq(subcommand, "USAGE")) {
        if (command->arg_count == 2) {
          const char *key = command->args[1].value;
          struct KVPair *kv = get_kv_from_cache(key);

          if (kv) {
            const uint32_t size = sizeof(struct KVPair *) + sizeof(struct KVPair) +
              ((kv->key.len + 1) * sizeof(char)) + get_value_size(kv->value, kv->type);

            char buf[14];
            const size_t nbytes = sprintf(buf, ":%d\r\n", size);
            _write(client, buf, nbytes);
          } else WRITE_NULL_REPLY(client);
        } else {
          WRONG_ARGUMENT_ERROR(client, "MEMORY USAGE", 12);
        }
      } else {
        WRONG_ARGUMENT_ERROR(client, "MEMORY", 6);
      }
    } else {
      WRONG_ARGUMENT_ERROR(client, "MEMORY", 6);
    }
  }
}

static struct Subcommand subcommands[] = {
  (struct Subcommand) {
    .name = "USAGE",
    .summary = "Gives how many bytes are used in the memory for the key.",
    .since = "0.1.2",
    .complexity = "O(1)"
  }
};

struct Command cmd_memory = {
  .name = "MEMORY",
  .summary = "Gives information about the memory.",
  .since = "0.1.2",
  .complexity = "O(1)",
  .subcommands = subcommands,
  .subcommand_count = 1,
  .run = run
};
