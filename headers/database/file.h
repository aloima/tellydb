#pragma once

#include <stdint.h>

typedef enum {
  BGSAVE_ALREADY_SAVING,
  BGSAVE_THREAD_FAILED,
  BGSAVE_SUCCESSFUL
} BackgroundSavingStatus;

int open_database_fd(uint32_t *server_age);
int close_database_fd();
int save_data(const uint32_t server_age);
BackgroundSavingStatus bg_save(const uint32_t server_age);
