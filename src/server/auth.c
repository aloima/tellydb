#include <telly.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <sys/types.h>
#include <unistd.h>

#include <openssl/crypto.h>
#include <openssl/kdf.h>
#include <openssl/provider.h>
#include <openssl/params.h>
#include <openssl/core_names.h>

static OSSL_LIB_CTX *libctx = NULL;
static OSSL_PROVIDER *prov = NULL;
static EVP_KDF_CTX *ctx = NULL;
static OSSL_PARAM params[5];

bool initialize_kdf() {
  if ((libctx = OSSL_LIB_CTX_new()) == NULL) {
    write_log(LOG_ERR, "Cannot load KDF library, because an error occured.");
    return false;
  }

  if ((prov = OSSL_PROVIDER_load(libctx, "default")) == NULL) {
    OSSL_LIB_CTX_free(libctx);
    write_log(LOG_ERR, "Cannot load KDF provider, because an error occured.");
    return false;;
  }

  EVP_KDF *kdf;
  if ((kdf = EVP_KDF_fetch(libctx, "HKDF", NULL)) == NULL) {
    OSSL_PROVIDER_unload(prov);
    OSSL_LIB_CTX_free(libctx);
    write_log(LOG_ERR, "Cannot fetch HKDF for deriving passwords, because allocation is failed.");
    return false;
  }

  if ((ctx = EVP_KDF_CTX_new(kdf)) == NULL) {
    OSSL_PROVIDER_unload(prov);
    OSSL_LIB_CTX_free(libctx);
    EVP_KDF_free(kdf);
    write_log(LOG_ERR, "Cannot create KDF context, because an error occured.");
    return false;
  }

  EVP_KDF_free(kdf);

  params[0] = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST, "SHA384", 0);
  params[1] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_INFO, "label", 5);
  params[2] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT, "salt", 4);
  params[4] = OSSL_PARAM_construct_end();

  return true;
}

void free_kdf() {
  if (ctx) EVP_KDF_CTX_free(ctx);
  if (prov) OSSL_PROVIDER_unload(prov);
  if (libctx) OSSL_LIB_CTX_free(libctx);
}

static bool password_derive(char *value, const size_t value_len, unsigned char *out) {
  params[3] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY, value, value_len);
  if (EVP_KDF_derive(ctx, out, 48, params) > 0) return true;

  write_log(LOG_ERR, "Cannot derive the password.");
  free_kdf();
  return false;
}

static inline void remove_password_from_clients(Password *password) {
  const Config *conf = get_server_config();
  Client *clients = get_clients();

  for (uint32_t i = 0; i < conf->max_clients; ++i) {
    if ((clients[i].id != -1) && clients[i].password == password) {
      clients[i].password = get_empty_password();
    }
  }
}

static Password **passwords = NULL;
static uint32_t password_count = 0;

Password **get_passwords() {
  return passwords;
}

uint32_t get_password_count() {
  return password_count;
}

uint16_t get_authorization_from_file(const int fd, char *block, const uint16_t block_size) {
  const uint8_t password_count_byte_count = block[10];
  memcpy(&password_count, block + 11, password_count_byte_count);

  off_t at = 11 + password_count_byte_count;
  off_t total = 0;

  if (password_count != 0) {
    Password *password;
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
            password = (passwords[password_at] = malloc(sizeof(Password)));

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

      if (password_at != password_count) at = 0;
      else break;
    } while (read(fd, block, block_size));
  } else {
    total = 11;
  }

  return (total % block_size);
}

int where_password(char *value, const size_t value_len) {
  unsigned char derived[48];
  if (!password_derive(value, value_len, derived)) return -1;

  for (uint32_t i = 0; i < password_count; ++i) {
    Password *password = passwords[i];
    if (memcmp(derived, password->data, 48) == 0) return i;
  }

  return -1;
}

bool edit_password(char *value, const size_t value_len, const uint32_t permissions) {
  const int at = where_password(value, value_len);
  if (at == -1) return false;

  passwords[at]->permissions = permissions;
  return true;
}

void add_password(Client *client, const string_t data, const uint8_t permissions) {
  Password *password;
  if (posix_memalign((void **) &password, 8, sizeof(Password)) != 0) {
    write_log(LOG_ERR, "Cannot create a password, out of memory.");
    return;
  }

  password_count += 1;

  if (password_count == 1) {
    passwords = malloc(sizeof(struct Password *));
  } else {
    passwords = realloc(passwords, password_count * sizeof(struct Password *));
  }

  if (passwords == NULL) {
    write_log(LOG_ERR, "Cannot create a password, out of memory.");
    return;
  } else if (password_count == 1) {
    client->password->permissions = 0; // Resets all client permissions via reference
    client->password = get_full_password(); // Give full permissions to client which added first password
  }

  password_derive(data.value, data.len, password->data);
  password->permissions = permissions;
  passwords[password_count - 1] = password;
}

void free_passwords() {
  if (password_count != 0 && passwords) {
    for (uint32_t i = 0; i < password_count; ++i) {
      free(passwords[i]);
    }

    free(passwords);
  }
}

bool remove_password(Client *executor, char *value, const size_t value_len) {
  if (password_count == 1) {
    const int at = where_password(value, value_len);
    if (at == -1) return false;

    executor->password = get_full_password();
    password_count = 0;

    remove_password_from_clients(passwords[at]);
    free(passwords[at]);
    free(passwords);

    return true;
  } else {
    const int at = where_password(value, value_len);
    if (at == -1) return false;

    remove_password_from_clients(passwords[at]);
    free(passwords[at]);
    password_count -= 1;

    memmove(passwords + at, passwords + at + 1, (password_count - at) * sizeof(Password *));
    passwords = realloc(passwords, password_count * sizeof(Password));

    return true;
  }
}
