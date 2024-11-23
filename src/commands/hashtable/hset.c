#include "../../../headers/server.h"
#include "../../../headers/database.h"
#include "../../../headers/commands.h"
#include "../../../headers/hashtable.h"
#include "../../../headers/utils.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

static void run(struct Client *client, commanddata_t *command, struct Password *password) {
  if (password->permissions & P_WRITE) {
    if (command->arg_count == 1 || (command->arg_count - 1) % 2 != 0) {
      if (client) WRONG_ARGUMENT_ERROR(client, "HSET", 4);
      return;
    }

    const string_t key = command->args[0];
    struct KVPair *kv = get_data(key.value);
    struct HashTable *table;

    if (kv) {
      if (kv->type == TELLY_HASHTABLE) {
        table = kv->value;
      } else {
        if (client) _write(client, "-Invalid type for 'HSET' command\r\n", 34);
        return;
      }
    } else {
      table = create_hashtable(16);
      set_data(kv, key, table, TELLY_HASHTABLE);
    }

    const uint32_t fv_count = (command->arg_count - 1) / 2;

    for (uint32_t i = 1; i <= fv_count; ++i) {
      const string_t name = command->args[(i * 2) - 1];
      const string_t input = command->args[i * 2];
      char *input_value = input.value;

      bool is_true = streq(input_value, "true");

      if (is_integer(input_value)) {
        const long number = atol(input.value);
        long *value = malloc(sizeof(long));
        memcpy(value, &number, sizeof(long));

        add_fv_to_hashtable(table, name, value, TELLY_NUM);
      } else if (is_true || streq(input_value, "false")) {
        bool *value = malloc(sizeof(bool));
        memset(value, is_true, sizeof(bool));

        add_fv_to_hashtable(table, name, value, TELLY_BOOL);
      } else if (streq(input_value, "null")) {
        add_fv_to_hashtable(table, name, NULL, TELLY_NULL);
      } else {
        string_t *value = malloc(sizeof(string_t));
        value->len = input.len;
        value->value = malloc(value->len);
        memcpy(value->value, input_value, value->len);

        add_fv_to_hashtable(table, name, value, TELLY_STR);
      }
    }

    if (client) {
      char buf[14];
      const size_t buf_len = sprintf(buf, ":%d\r\n", fv_count);

      _write(client, buf, buf_len);
    }
  } else if (client) {
    _write(client, "-Not allowed to use this command, need P_WRITE\r\n", 48);
  }
}

const struct Command cmd_hset = {
  .name = "HSET",
  .summary = "Sets field(s) of the hash table.",
  .since = "0.1.3",
  .complexity = "O(N) where N is written field name-value pair count",
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
