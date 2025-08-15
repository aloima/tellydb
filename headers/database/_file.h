#pragma once

#include "../config.h"

#include <stdint.h>

const bool open_database_fd(struct Configuration *conf, uint32_t *server_age);
const bool close_database_fd();
const bool save_data(const uint32_t server_age);
const bool bg_save(const uint32_t server_age);
