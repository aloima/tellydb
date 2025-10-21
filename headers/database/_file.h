#pragma once

#include <stdint.h>

bool open_database_fd(uint32_t *server_age);
bool close_database_fd();
bool save_data(const uint32_t server_age);
bool bg_save(const uint32_t server_age);
