#pragma once

#include <stdint.h>

int open_database_fd(uint32_t *server_age);
int close_database_fd();
int save_data(const uint32_t server_age);
bool bg_save(const uint32_t server_age);
