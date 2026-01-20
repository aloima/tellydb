#include <telly.h>

#include <stddef.h>

static string_t run(struct CommandEntry *entry) {
  if (entry->args->count != 1) {
    PASS_NO_CLIENT(entry->client);
    return WRONG_ARGUMENT_ERROR("LPOP");
  }

  const string_t key = entry->args->data[0];
  const struct KVPair *kv = get_data(entry->database, key);

  if (!kv) {
    PASS_NO_CLIENT(entry->client);
    return RESP_NULL(entry->client->protover);
  }

  if (kv->type != TELLY_LIST) {
    PASS_NO_CLIENT(entry->client);
    return INVALID_TYPE_ERROR("LPOP");
  }

  struct List *list = kv->value;
  struct ListNode *node = list->begin;
  string_t response = EMPTY_STRING();

  if (entry->client) {
    response = write_value(node->value, node->type, entry->client->protover, entry->client->write_buf);
  }

  if (list->size == 1) {
    delete_data(entry->database, key);
  } else {
    list->begin = list->begin->next;
    list->begin->prev = NULL;

    list->size -= 1;
    free_listnode(node);
  }

  return response;
}

const struct Command cmd_lpop = {
  .name = "LPOP",
  .summary = "Removes and returns first element of the list.",
  .since = "0.1.3",
  .complexity = "O(1)",
  .permissions = (P_READ | P_WRITE),
  .flags.value = CMD_FLAG_DATABASE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run
};
