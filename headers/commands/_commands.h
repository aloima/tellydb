#pragma once

#include "../server/_client.h"
#include "../database/_database.h"
#include "../resp.h"
#include "../auth.h"

#include <stdint.h>
#include <stddef.h>

#define WRONG_ARGUMENT_ERROR(name)     RESP_ERROR_MESSAGE("Wrong argument count for '" name "' command")
#define INVALID_TYPE_ERROR(name)       RESP_ERROR_MESSAGE("Invalid type for '" name "' command")
#define MISSING_SUBCOMMAND_ERROR(name) RESP_ERROR_MESSAGE("Missing subcommand for '" name "' command")
#define INVALID_SUBCOMMAND_ERROR(name) RESP_ERROR_MESSAGE("Invalid subcommand for '" name "' command")
#define PASS_COMMAND()                 return EMPTY_STRING()
#define PASS_NO_CLIENT(client) \
  if (!(client)) { \
    PASS_COMMAND(); \
  }

#define CREATE_COMMAND_ENTRY(_client, _data, _database, _password, _buffer) ({\
  (struct CommandEntry) {\
    .client = (_client),\
    .data = (_data),\
    .database = (_database),\
    .password = (_password),\
    .buffer = (_buffer)\
  };\
})

struct CommandIndex {
  const char *name;
  uint32_t idx;
};

const struct CommandIndex *get_command_index(const char *str, size_t len);

struct CommandEntry {
  struct Database *database;
  struct Client *client;
  struct Password *password;
  commanddata_t *data;
  char *buffer;
};

struct Subcommand {
  char *name;
  char *summary;
  char *since;
  char *complexity;
};

struct Command {
  char *name;
  char *summary;
  char *since;
  char *complexity;
  uint64_t permissions;
  string_t (*run)(struct CommandEntry entry);
  struct Subcommand *subcommands;
  uint32_t subcommand_count;
};

struct Command *load_commands();
struct Command *get_commands();
uint32_t get_command_count();
void free_commands();
