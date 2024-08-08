#include "resp.h"

#include <stdint.h>

#ifndef COMMANDS_H
  #define COMMANDS_H

  struct Command {
    char *name;
    void (*run)(int connfd, respdata_t data);
  };

  void execute_commands(int connfd, respdata_t data);

  void load_commands();
  struct Command *get_commands();
  uint32_t get_command_count();

  extern struct Command cmd_command;
  extern struct Command cmd_get;
#endif
