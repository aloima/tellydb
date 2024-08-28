#include "resp.h"
#include "server.h"

#include <stdint.h>

#ifndef COMMANDS_H
  #define COMMANDS_H

  struct Command {
    char *name;
    char *summary;
    void (*run)(struct Client *client, respdata_t *data, struct Configuration *conf);
  };

  void execute_command(struct Client *client, respdata_t *data, struct Configuration *conf);

  void load_commands();
  struct Command *get_commands();
  uint32_t get_command_count();
  void free_commands();

  extern struct Command cmd_client;
  extern struct Command cmd_command;
  extern struct Command cmd_decr;
  extern struct Command cmd_get;
  extern struct Command cmd_incr;
  extern struct Command cmd_info;
  extern struct Command cmd_set;
  extern struct Command cmd_type;
#endif
