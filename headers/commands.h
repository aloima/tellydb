#pragma once

#include "server.h"

#include <stdint.h>

#define WRONG_ARGUMENT_ERROR(client, name, len) (_write((client), "-Wrong argument count for '" name "' command\r\n", 38 + (len)))

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
  void (*run)(struct Client *client, commanddata_t *command);
  struct Subcommand *subcommands;
  uint32_t subcommand_count;
};

void execute_command(struct Client *client, commanddata_t *command);

void load_commands();
struct Command *get_commands();
uint32_t get_command_count();
void free_commands();


/* DATABASE COMMANDS */
extern struct Command cmd_bgsave;
extern struct Command cmd_dbsize;
extern struct Command cmd_lastsave;
extern struct Command cmd_save;
/* /DATABASE COMMANDS */

/* GENERIC COMMANDS */
extern struct Command cmd_age;
extern struct Command cmd_client;
extern struct Command cmd_command;
extern struct Command cmd_hello;
extern struct Command cmd_info;
extern struct Command cmd_ping;
extern struct Command cmd_time;
/* /GENERIC COMMANDS */

/* HASHTABLE COMMANDS */
extern struct Command cmd_hdel;
extern struct Command cmd_hget;
extern struct Command cmd_hlen;
extern struct Command cmd_hset;
extern struct Command cmd_htype;
/* /HASHTABLE COMMANDS */

/* KV COMMANDS */
extern struct Command cmd_decr;
extern struct Command cmd_get;
extern struct Command cmd_exists;
extern struct Command cmd_incr;
extern struct Command cmd_set;
extern struct Command cmd_type;
/* /KV COMMANDS */

/* LIST COMMANDS */
extern struct Command cmd_lindex;
extern struct Command cmd_llen;
extern struct Command cmd_lpop;
extern struct Command cmd_lpush;
extern struct Command cmd_rpop;
extern struct Command cmd_rpush;
/* /LIST COMMANDS */
