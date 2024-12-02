#include "../../headers/server.h"
#include "../../headers/utils.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <openssl/core_names.h>
#include <openssl/kdf.h>
#include <openssl/obj_mac.h>
#include <openssl/provider.h>

#include <unistd.h>

static OSSL_LIB_CTX *libctx;
static OSSL_PROVIDER *prov; 
static EVP_KDF_CTX *ctx;
static OSSL_PARAM params[5];

bool initialize_kdf() {
  libctx = OSSL_LIB_CTX_new();
  prov = OSSL_PROVIDER_load(libctx, "default");

  EVP_KDF *kdf = EVP_KDF_fetch(libctx, "HKDF", NULL);
  ctx = EVP_KDF_CTX_new(kdf);
  EVP_KDF_free(kdf);

  params[0] = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST, "SHA384", 0);
  params[1] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_INFO, "label", 5);
  params[2] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT, "salt", 4);
  params[4] = OSSL_PARAM_construct_end();

  return true;
}

void free_kdf() {
  EVP_KDF_CTX_free(ctx);
  OSSL_PROVIDER_unload(prov);
  OSSL_LIB_CTX_free(libctx);
}

static bool password_derive(char *value, const size_t value_len, unsigned char *out) {
  params[3] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY, value, value_len);

  if (EVP_KDF_derive(ctx, out, 48, params) <= 0) {
    write_log(LOG_ERR, "Cannot derive the password.");
    free_kdf();
    return false;
  } else return true;
}

void remove_password_from_clients(struct Password *password) {
  struct Client **clients = get_clients();
  const uint32_t client_count = get_client_count();

  for (uint32_t i = 0; i < client_count; ++i) {
    struct Client *client = clients[i];

    if (client->password == password) {
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

uint16_t get_authorization_from_file(const int fd, char *block, const uint16_t block_size) {
  const uint8_t password_count_byte_count = block[10];
  memcpy(&password_count, block + 11, password_count_byte_count);

  off64_t at = 11 + password_count_byte_count;
  off_t total = at;

  if (password_count != 0) {
    struct Password *password;
    uint32_t password_at = 0;
    passwords = malloc(password_count * sizeof(struct Password *));

    /*
      Interrupted operations

      0: No interrupted operation
      1: Retrieving value
      2: Retrieving permissions
    */
    uint8_t op = 0;
    uint32_t overflow_len = 0;

    do {
      switch (op) {
        case 1: {
          password = passwords[password_at];
          memcpy(password->data + (48 - overflow_len), block, overflow_len);

          at = overflow_len;
          break;
        }

        case 2: {
          password = passwords[password_at];
          password->permissions = block[0];

          op = 0;
          at += 1;
          password_at += 1;
          break;
        }
      }

      while (password_at != password_count) {
        {
          if (op < 1) {
            password = (passwords[password_at] = malloc(sizeof(struct Password)));

            if ((at + 48) > block_size) {
              const uint32_t remaining = (block_size - at);
              memcpy(password->data, block + at, remaining);
              overflow_len = 48 - remaining;
              op = 1;
              break;
            } else {
              memcpy(password->data, block + at, 48);
              at += 48;
            }
          }
        }

        if (op < 2) {
          if ((at + 1) > block_size) {
            op = 2;
            break;
          } else {
            password->permissions = block[at];
            at += 1;
            op = 0;
          }
        }

        password_at += 1;
      }

      total += at;
      at = 0;
    } while (read(fd, block, block_size));
  }

  return (total % block_size);
}

int32_t where_password(char *value, const size_t value_len) {
  unsigned char derived[48];
  if (!password_derive(value, value_len, derived)) return -1;

  for (uint32_t i = 0; i < password_count; ++i) {
    struct Password *password = passwords[i];
    if (memcmp(derived, password->data, 48) == 0) return i;
  }

  return -1;
}

struct Password *get_password(char *value, const size_t value_len) {
  unsigned char derived[48];
  if (!password_derive(value, value_len, derived)) return NULL;

  for (uint32_t i = 0; i < password_count; ++i) {
    struct Password *password = passwords[i];
    if (memcmp(derived, password->data, 48) == 0) return password;
  }

  return NULL;
}

bool edit_password(char *value, const size_t value_len, const uint32_t permissions) {
  struct Password *password = get_password(value, value_len);

  if (password) {
    password->permissions = permissions;
    return true;
  } else return false;
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

  password_derive(data.value, data.len, password->data);
  password->permissions = permissions;
}

void free_password(struct Password *password) {
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

bool remove_password(struct Client *executor, char *value, const size_t value_len) {
  if (password_count == 1) {
    if (where_password(value, value_len) == 0) {
      struct Password *password = passwords[0];
      remove_password_from_clients(password);

      executor->password = get_full_password();
      password_count = 0;

      free_password(password);
      free(passwords);

      return true;
    } else return false;
  } else {
    const int32_t at = where_password(value, value_len);

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
