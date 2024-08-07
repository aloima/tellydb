#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#ifndef TELLY_H
  #define TELLY_H

  #define streq(s1, s2) (strcmp((s1), (s2)) == 0)

  struct Configuration {
    uint16_t port;
  };

  #define RDT_SSTRING '+'
  #define RDT_BSTRING '$'
  #define RDT_ARRAY '*'
  #define RDT_ERR '-'

  typedef struct String {
    char *data;
    size_t len;
  } string_t;

  typedef struct RESPData {
    uint8_t type;

    union {
      string_t string;
      bool boolean;
      int32_t integer;
      double doubl;
      struct RESPData *array;
    } value;

    uint32_t count;
  } respdata_t;

  struct Command {
    char *name;
    void (*run)(int connfd, respdata_t data);
  };

  void start_server(struct Configuration conf);
  respdata_t get_resp_data(int connfd);
  void execute_commands(int connfd, respdata_t data);

  struct Configuration get_configuration(const char *filename);
  struct Configuration get_default_configuration();
  void get_configuration_string(char *buf, struct Configuration conf);

  void client_error();

  void load_commands();
  struct Command *get_commands();
  uint32_t get_command_count();

  extern struct Command cmd_command;
  extern struct Command cmd_get;
#endif
