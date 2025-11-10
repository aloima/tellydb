#include <telly.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static string_t run(struct CommandEntry *entry) {
  if (entry->data->arg_count != 2) {
    PASS_NO_CLIENT(entry->client);
    return WRONG_ARGUMENT_ERROR("APPEND");
  }

  const string_t key = entry->data->args[0];
  const struct KVPair *kv = get_data(entry->database, key);

  if (kv) {
    if (kv->type != TELLY_STR) {
      PASS_NO_CLIENT(entry->client);
      return INVALID_TYPE_ERROR("APPEND");
    }

    const string_t arg = entry->data->args[1];

    string_t *string = kv->value;
    string->value = realloc(string->value, string->len + arg.len);
    memcpy(string->value + string->len, arg.value, arg.len);
    string->len += arg.len;

    PASS_NO_CLIENT(entry->client);
    const size_t nbytes = create_resp_integer(entry->buffer, string->len);
    return CREATE_STRING(entry->buffer, nbytes);
  } else {
    const string_t arg = entry->data->args[1];

    string_t *string = malloc(sizeof(string_t));
    string->len = arg.len;
    string->value = malloc(string->len);
    memcpy(string->value, arg.value, string->len);
    set_data(entry->database, NULL, key, string, TELLY_STR);

    PASS_NO_CLIENT(entry->client);
    const size_t nbytes = create_resp_integer(entry->buffer, string->len);
    return CREATE_STRING(entry->buffer, nbytes);
  }
}

const struct Command cmd_append = {
  .name = "APPEND",
  .summary = "Appends string to existed value. If key is not exist, creates a new one.",
  .since = "0.1.7",
  .complexity = "O(1)",
  .permissions = (P_READ | P_WRITE),
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
