#pragma once

#include "_client.h"
#include "../resp.h"

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

#define WRITE_OK(client)                     _write((client), RDT_SSTRING_SL "OK\r\n", 5)
#define WRITE_ERROR(client)                  _write((client), RDT_ERR_SL "ERROR\r\n", 8)
#define WRITE_ERROR_MESSAGE(client, message) _write((client), RDT_ERR_SL message "\r\n", sizeof(message) + 2)
