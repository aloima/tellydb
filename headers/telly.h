#include "commands.h"
#include "config.h"
#include "resp.h"
#include "database.h"
#include "btree.h"
#include "utils.h"

#include <string.h>

#ifndef TELLY_H
  #define TELLY_H

  #define streq(s1, s2) (strcmp((s1), (s2)) == 0)

  void start_server(struct Configuration conf);
  void client_error();
#endif
