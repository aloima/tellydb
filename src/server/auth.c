#include "../../headers/server.h"
#include "../../headers/utils.h"

#include <stdint.h>

#include <unistd.h>

static struct Password **passwords;
static uint32_t password_count = 0;

struct Password **get_passwords() {
  return passwords;
}

uint32_t get_password_count() {
  return password_count;
}

off_t get_authorization_from_file(const int fd) {
  lseek(fd, 10, SEEK_SET);

  uint8_t password_count_byte_count;
  read(fd, &password_count_byte_count, 1);
  read(fd, &password_count, password_count_byte_count);

  passwords = malloc(password_count * sizeof(struct Password *));

  for (uint32_t i = 0; i < password_count; ++i) {
    struct Password *password = (passwords[i] = malloc(sizeof(struct Password)));
    string_t *data = &password->data;

    // String length specifier
    {
      uint8_t first;
      read(fd, &first, 1);

      const uint8_t byte_count = first >> 6;
      data->len = 0;
      read(fd, &data->len, byte_count);

      data->len = (data->len << 6) | (first & 0b111111);
      data->value = malloc(data->len + 1);
    }

    read(fd, data->value, data->len);
    data->value[data->len] = '\0';

    read(fd, &password->permissions, 1);
  }

  return lseek(fd, 0, SEEK_CUR);
}

int32_t where_password(const char *value) {
  for (uint32_t i = 0; i < password_count; ++i) {
    if (streq(value, passwords[i]->data.value)) return i;
  }

  return -1;
}

bool edit_password(const char *value, const uint32_t permissions) {
  for (uint32_t i = 0; i < password_count; ++i) {
    struct Password *password = passwords[i];

    if (streq(value, password->data.value)) {
      password->permissions = permissions;
      return true;
    }
  }

  return false;
}

void add_password(struct Client *client, const string_t data, const uint8_t permissions) {
  struct Password *password = malloc(sizeof(struct Password));
  password_count += 1;

  if (password_count == 1) {
    passwords = malloc(sizeof(struct Password *));
    passwords[0] = password;

    client->password->permissions = 0; // Resets all client permissions via reference
    client->password = get_full_password(); // Give full permissions to client which added first password
  } else {
    passwords = realloc(passwords, password_count * sizeof(struct Password));
    passwords[password_count - 1] = password;
  }

  password->permissions = permissions;
  password->data.len = data.len;
  password->data.value = malloc(data.len + 1);
  memcpy(password->data.value, data.value, data.len + 1);
}

void free_password(struct Password *password) {
  free(password->data.value);
  free(password);
}

void free_passwords() {
  for (uint32_t i = 0; i < password_count; ++i) {
    free_password(passwords[i]);
  }

  free(passwords);
}

bool remove_password(struct Client *executor, const char *value) {
  if (password_count == 1) {
    if (where_password(value) == 0) {
      executor->password = get_full_password();
      password_count = 0;

      free_password(passwords[0]);
      free(passwords);

      return true;
    } else return false;
  } else {
    const int32_t at = where_password(value);

    if (at == -1) return false;
    else {
      struct Client **clients = get_clients();
      const uint32_t client_count = get_client_count();

      for (uint32_t i = 0; i < client_count; ++i) {
        struct Client *client = clients[i];

        if (streq(client->password->data.value, value)) {
          client->password = NULL;
        }
      }

      free_password(passwords[at]);
      password_count -= 1;

      memcpy(passwords + at, passwords + at + 1, (password_count - at) * sizeof(struct Password));
      passwords = realloc(passwords, password_count * sizeof(struct Password));

      return true;
    }
  }
}
