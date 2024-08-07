#include <stdint.h>
#include <string.h>

#ifndef TELLY_H
  #define TELLY_H

  #define streq(s1, s2) (strcmp((s1), (s2)) == 0)

  struct Configuration {
    uint16_t port;
  };

  void start_server(struct Configuration conf);
  struct Configuration get_configuration(const char *filename);
#endif
