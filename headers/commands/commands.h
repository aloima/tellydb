#pragma once

#include "../database/database.h"
#include "../server/client.h"
#include "../resp.h"
#include "../auth.h"

#include <stdint.h>
#include <stddef.h>

enum CommandFlags {
  CMD_FLAG_NO_FLAG,
  CMD_FLAG_DATABASE, // affects on databases, writing/deleting/updating/selecting database, not includes file operations nor getting
  CMD_FLAG_WAITING_TX
};

struct CommandIndex {
  const char *name;
  uint32_t idx;
};

const struct CommandIndex *get_command_index(const char *str, size_t len);

struct CommandEntry {
  struct Database *database;
  Client *client;
  Password *password;
  commanddata_t *data;
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
  uint8_t flags;
  string_t (*run)(struct CommandEntry *entry);
  struct Subcommand *subcommands;
  uint32_t subcommand_count;
};

struct Command *load_commands();
struct Command *get_commands();
uint32_t get_command_count();
void free_commands();

#include "macros.h" // IWYU pragma: export
#include "data.h"   // IWYU pragma: export
