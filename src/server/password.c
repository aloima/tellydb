#include <telly.h>

#include <stdbool.h>
#include <stdlib.h>

struct Password *default_password = NULL,
                *empty_password = NULL,
                *full_password = NULL;

static bool create_constant_password(struct Password **password, uint64_t permissions) {
  if (posix_memalign((void **) password, 64, sizeof(struct Password)) != 0) {
    write_log(LOG_ERR, "Cannot create constant passwords, out of memory.");
    return false;
  }

  (*password)->permissions = permissions;
  return true;
}

bool create_constant_passwords() {
  constexpr uint64_t permissions = (P_READ | P_WRITE | P_CLIENT | P_CONFIG | P_AUTH | P_SERVER);
  if (!create_constant_password(&default_password, permissions)) return false;
  if (!create_constant_password(&empty_password, 0)) return false;
  if (!create_constant_password(&full_password, permissions)) return false;

  return true;
}

void free_constant_passwords() {
  if (default_password) free(default_password);
  if (empty_password) free(empty_password);
  if (full_password) free(full_password);
}

struct Password *get_empty_password() {
  return empty_password;
}

struct Password *get_full_password() {
  return full_password;
}

struct Password *get_default_password() {
  return default_password;
}
