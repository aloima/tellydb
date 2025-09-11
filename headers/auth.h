// Includes password methods, password structure and permissions enum

#pragma once

#include "./server/server.h"
#include "utils.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

enum Permissions {
  P_NONE   = 0b00000000,
  P_READ   = 0b00000001,
  P_WRITE  = 0b00000010,
  P_CLIENT = 0b00000100,
  P_CONFIG = 0b00001000,
  P_AUTH   = 0b00010000,
  P_SERVER = 0b00100000,
};

struct Password {
  unsigned char data[48];
  uint8_t permissions;
  uint8_t _padding[3];
};

bool create_constant_passwords();
void free_constant_passwords();
struct Password *get_full_password();
struct Password *get_empty_password();

bool initialize_kdf();
void free_kdf();

struct Password **get_passwords();
uint32_t get_password_count();
uint16_t get_authorization_from_file(const int fd, char *block, const uint16_t block_size);
void free_passwords();

void add_password(struct Client *client, const string_t data, const uint8_t permissions);
bool remove_password(struct Client *executor, char *value, const size_t value_len);
int32_t where_password(char *value, const size_t value_len);
struct Password *get_password(char *value, const size_t value_len);
bool edit_password(char *value, const size_t value_len, const uint32_t permissions);
