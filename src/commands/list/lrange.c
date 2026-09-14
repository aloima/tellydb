#include <telly.h>

static void get_keys(CommandEntry *entry) {
  if (entry->args->count != 3) return;
  ASSERT(insert_into_vector(server->keyspace, &entry->args->data[0]), ==, true);
}



// Normalizes index, removes negativity and limits value
static inline uint64_t normalize_index(uint64_t list_size, string_t string) {
  if (string.value[0] == '-') {
    string.value[0] = '+';

    const uint64_t index = atoull_s(string);
    ASSERT(errno, !=, EINVAL);

    if (index >= list_size) { // Out of bounds for negative indexes
      return list_size;
    } else {
      return (list_size - index);
    }
  } else {
    const uint64_t index = atoull_s(string);
    ASSERT(errno, !=, EINVAL);

    if (index >= list_size) {
      return (list_size - 1);
    } else {
      return index;
    }
  }
}

static inline LLSearchDirection determine_dir(const uint64_t start, const uint64_t end, const uint64_t size) {
  if (start <= (size - end)) {
    return LL_FRONT;
  } else {
    return LL_BACK;
  }
}

static inline void collect_values(LinkedList *list, Value **values, const uint64_t count, const uint64_t start, const uint64_t end) {
  const LLSearchDirection dir = determine_dir(start, end, list->size);

  // Guaranteed that non-nullable
  if (dir == LL_FRONT) {
    LinkedListNode *from = ll_get_from_index(list, start, LL_FRONT);

    for (uint64_t i = 0; i < count; ++i) {
      values[i] = (Value *) from->data;
      from = from->next;
    }
  } else if (dir == LL_BACK) {
    LinkedListNode *from = ll_get_from_index(list, list->size - end - 1, LL_FRONT);
    uint64_t i = count - 1;

    while (true) {
      values[i] = (Value *) from->data;
      from = from->prev;

      if (i == 0) break;
      i -= 1;
    }
  }
}

#define EMPTY_ARRAY() CREATE_SIZED_STRING("*0\r\n")

static string_t run(CommandEntry *entry) {
  PASS_NO_CLIENT(entry->client);

  if (entry->args->count != 3) {
    return WRONG_ARGUMENT_ERROR("LRANGE");
  }

  string_t start_str = entry->args->data[1];
  string_t end_str = entry->args->data[2];

  if (!try_parse_integer(start_str)) {
    return RESP_ERROR_MESSAGE("Second argument must be an integer");
  }

  if (!try_parse_integer(end_str)) {
    return RESP_ERROR_MESSAGE("Third argument must be an integer");
  }

  const KeyValue *kv = get_data(entry->database, entry->args->data[0]);

  if (!kv) {
    return EMPTY_ARRAY();
  } else if (kv->value.type != TELLY_LIST) {
    return INVALID_TYPE_ERROR("LRANGE");
  }

  LinkedList *list = (LinkedList *) kv->value.data;
  const uint64_t list_size = list->size;

  if (list_size == 0) {
    return EMPTY_ARRAY();
  }

  const uint64_t start = normalize_index(list_size, start_str);
  const uint64_t end = normalize_index(list_size, end_str);

  // Out of bounds behavior
  if (start >= list_size || end <= start || end == list_size) {
    return EMPTY_ARRAY();
  }

  const uint64_t count = (end - start + 1);
  Value *values[count];

  collect_values(list, values, count, start, end);

  uint64_t at = sprintf(entry->client->write_buf, "*%" PRIu64 "\r\n", count);

  for (uint64_t i = 0; i < count; ++i) {
    Value *value = values[i];
    string_t res = write_value(value->data, value->type, entry->client->protover, entry->client->write_buf + at);
    at += res.len;
  }

  return CREATE_STRING(entry->client->write_buf, at);
}

const Command cmd_lrange = {
  .name = "LRANGE",
  .summary = "Returns the elements between two indexes from the list.",
  .since = "1.2.2",
  .complexity = "O(N + S)",
  .permissions = P_READ,
  .flags.value = CMD_FLAG_ACCESS_DATABASE,
  .subcommands = NULL,
  .subcommand_count = 0,
  .run = run,
  .get_keys = get_keys
};
