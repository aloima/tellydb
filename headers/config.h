#include <stdint.h>

#ifndef CONFIG_H
  #define CONFIG_H

  struct Configuration {
    uint16_t port;
  };

  struct Configuration get_configuration(const char *filename);
  struct Configuration get_default_configuration();
  void get_configuration_string(char *buf, struct Configuration conf);
#endif
