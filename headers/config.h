#include <stdint.h>

#ifndef CONFIG_H
  #define CONFIG_H

  struct Configuration {
    uint16_t port;
    uint16_t max_clients;
    uint8_t allowed_log_levels;
    uint32_t max_log_len;
    char *data_file;
    char *log_file;
  };

  struct Configuration *get_configuration(const char *filename);
  struct Configuration get_default_configuration();
  uint32_t get_configuration_string(char *buf, struct Configuration conf);
#endif
