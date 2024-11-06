#include "../../headers/server.h"
#include "../../headers/utils.h"

#include <stdint.h>

#include <unistd.h>

void remove_password_from_clients(struct Password *password) {
  struct Client **clients = get_clients();
  const uint32_t client_count = get_client_count();

  for (uint32_t i = 0; i < client_count; ++i) {
    struct Client *client = clients[i];

    if (client->password == (password)) {
      client->password = get_empty_password();
    }
  }
}

static struct Password **passwords;
static uint32_t password_count = 0;

struct Password **get_passwords() {
  return passwords;
}

uint32_t get_password_count() {
  return password_count;
}

static void get_password_from_file(const int fd, const uint32_t i) {
  struct Password *password = (passwords[i] = malloc(sizeof(struct Password)));

  read_string_from_file(fd, &password->data, true, true);

  read(fd, password->salt, 2);
  password->salt[2] = '\0';

  read(fd, &password->permissions, 1);
}

off_t get_authorization_from_file(const int fd) {
  lseek(fd, 10, SEEK_SET);

  uint8_t password_count_byte_count;
  read(fd, &password_count_byte_count, 1);
  read(fd, &password_count, password_count_byte_count);

  if (password_count != 0) {
    passwords = malloc(password_count * sizeof(struct Password *));
    get_password_from_file(fd, 0);

    for (uint32_t i = 1; i < password_count; ++i) {
      get_password_from_file(fd, i);
    }
  }

  return lseek(fd, 0, SEEK_CUR);
}

int32_t where_password(const char *value) {
  for (uint32_t i = 0; i < password_count; ++i) {
    struct Password *password = passwords[i];
    const char *hashed = crypt(value, password->salt);

    if (streq(hashed, password->data.value)) return i;
  }

  return -1;
}

struct Password *get_password(const char *value) {
  for (uint32_t i = 0; i < password_count; ++i) {
    struct Password *password = passwords[i];
    const char *hashed = crypt(value, password->salt);

    if (streq(hashed, password->data.value)) return password;
  }

  return NULL;
}

bool edit_password(const char *value, const uint32_t permissions) {
  for (uint32_t i = 0; i < password_count; ++i) {
    struct Password *password = passwords[i];
    const char *hashed = crypt(value, password->salt);

    if (streq(hashed, password->data.value)) {
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

  generate_random_string(password->salt, 2);
  const char *hashed = crypt(data.value, password->salt);
  password->permissions = permissions;

  const uint32_t size = (password->data.len = strlen(hashed)) + 1;
  password->data.value = malloc(size);
  memcpy(password->data.value, hashed, size);
}

void free_password(struct Password *password) {
  free(password->data.value);
  free(password);
}

void free_passwords() {
  if (password_count != 0) {
    free_password(passwords[0]);

    for (uint32_t i = 1; i < password_count; ++i) {
      free_password(passwords[i]);
    }

    free(passwords);
  }
}

bool remove_password(struct Client *executor, const char *value) {
  if (password_count == 1) {
    if (where_password(value) == 0) {
      struct Password *password = passwords[0];
      remove_password_from_clients(password);

      executor->password = get_full_password();
      password_count = 0;

      free_password(password);
      free(passwords);

      return true;
    } else return false;
  } else {
    const int32_t at = where_password(value);

    if (at == -1) return false;
    else {
      struct Password *password = passwords[at];
      remove_password_from_clients(password);

      free_password(password);
      password_count -= 1;

      memcpy(passwords + at, passwords + at + 1, (password_count - at) * sizeof(struct Password));
      passwords = realloc(passwords, password_count * sizeof(struct Password));

      return true;
    }
  }
}
