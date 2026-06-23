#pragma once

#include <stdint.h>

#define DATABASE_FILE_CONSTANT ((uint16_t) 0x1810)

typedef enum {
  BGSAVE_THREAD_FAILED  = -1,
  BGSAVE_ALREADY_SAVING = 0,
  BGSAVE_SUCCESSFUL     = 1
} BackgroundSavingStatus;

int open_database_fd(uint32_t *server_age);
int close_database_fd();
int save_data(const uint32_t server_age);
BackgroundSavingStatus bg_save(const uint32_t server_age);
