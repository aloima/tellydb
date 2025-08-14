#pragma once

#include "../server/_client.h"
#include "../database/_database.h"
#include "../resp.h"
#include "../auth.h"

#include <stdint.h>

#define WRONG_ARGUMENT_ERROR(client, name) WRITE_ERROR_MESSAGE((client), "Wrong argument count for '" name "' command")
#define INVALID_TYPE_ERROR(client, name) WRITE_ERROR_MESSAGE((client), "Invalid type for '" name "' command")
#define MISSING_SUBCOMMAND_ERROR(client, name) WRITE_ERROR_MESSAGE((client), "Missing subcommand for '" name "' command")
#define INVALID_SUBCOMMAND_ERROR(client, name) WRITE_ERROR_MESSAGE((client), "Invalid subcommand for '" name "' command")

struct CommandEntry {
  struct Database *database;
  struct Client *client;
  struct Password *password;
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
  void (*run)(struct CommandEntry entry);
  struct Subcommand *subcommands;
  uint32_t subcommand_count;
};

struct Command *load_commands();
struct Command *get_commands();
uint32_t get_command_count();
void free_commands();
