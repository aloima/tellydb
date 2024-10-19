#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct Configuration {
  uint16_t port;
  uint16_t max_clients;
  uint8_t allowed_log_levels;
  uint32_t max_log_len;
  int32_t max_log_lines;
  char data_file[49];
  char log_file[49];

  bool tls;
  char cert[49];
  char private_key[49];

  bool default_conf;
};

struct Configuration *get_configuration(const char *filename);
struct Configuration get_default_configuration();
size_t get_configuration_string(char *buf, struct Configuration conf);
void free_configuration(struct Configuration *conf);
