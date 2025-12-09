#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
  uint16_t port;
  uint16_t max_clients;
  uint32_t max_transaction_blocks;
  uint8_t allowed_log_levels;
  int32_t max_log_lines;
  char data_file[49];
  char log_file[49];
  char database_name[65];

  bool tls;
  char cert[49];
  char private_key[49];
} Config;

Config *get_config(const char *filename);
Config *get_default_config();
size_t get_config_string(char *buf, Config *conf);
void free_config(Config *conf);
