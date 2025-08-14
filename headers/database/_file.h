#pragma once

#include "../config.h"

#include <stdint.h>

bool open_database_fd(struct Configuration *conf, uint32_t *server_age);
void close_database_fd();
