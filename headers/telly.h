#if defined(__linux__) && !defined(_GNU_SOURCE)
  #define _GNU_SOURCE
#endif

#include "commands.h"
#include "config.h"
#include "resp.h"
#include "database.h"
#include "btree.h"
#include "utils.h"
#include "server.h"
#include "hashtable.h"

#include <string.h> // IWYU pragma: keep

#ifndef TELLY_H
  #define TELLY_H

  #define streq(s1, s2) (strcmp((s1), (s2)) == 0)

  void client_error();
#endif
