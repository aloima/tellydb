#pragma once

#include "_client.h"
#include "../resp.h"
#include "../utils.h" // IWYU pragma: export

#include <stddef.h>

#include <unistd.h>

#include <openssl/ssl.h>

static inline int _read(struct Client *client, char *buf, const size_t nbytes) {
  return (client->ssl ? SSL_read(client->ssl, buf, nbytes) : read(client->connfd, buf, nbytes));
}

static inline int _write(struct Client *client, char *buf, const size_t nbytes) {
  return (client->ssl ? SSL_write(client->ssl, buf, nbytes) : write(client->connfd, buf, nbytes));
}

#define WRITE_NULL_REPLY(client) \
  switch ((client)->protover) {\
    case RESP2:\
      _write((client), "$-1\r\n", 5);\
      break;\
\
    case RESP3:\
      _write((client), "_\r\n", 3);\
      break;\
  }

#define RESP_NULL(protover) ({\
  string_t response;\
  switch (protover) {\
    case RESP2:\
      response = CREATE_STRING("$-1\r\n", 5);\
      break;\
\
    case RESP3:\
      response = CREATE_STRING("_\r\n", 3);\
      break;\
  }\
\
  response;\
})

#define RESP_OK()                   CREATE_STRING(RDT_SSTRING_SL "OK\r\n",       5)
#define RESP_OK_MESSAGE(message)    CREATE_STRING(RDT_SSTRING_SL message "\r\n", sizeof(message) + 2)
#define RESP_ERROR()                CREATE_STRING(RDT_ERR_SL     "ERROR\r\n",    8)
#define RESP_ERROR_MESSAGE(message) CREATE_STRING(RDT_ERR_SL     message "\r\n", sizeof(message) + 2)
