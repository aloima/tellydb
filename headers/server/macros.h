#pragma once

#include "client.h"
#include "../resp.h"
#include "../utils/utils.h" // IWYU pragma: export

#include <stddef.h>

#include <unistd.h>

#include <openssl/ssl.h>

static inline int _read(Client *client, char *buf, const size_t nbytes) {
  return (!client->ssl ? read(client->connfd, buf, nbytes) : SSL_read(client->ssl, buf, nbytes));
}

static inline int _write(Client *client, char *buf, const size_t nbytes) {
  return (!client->ssl ? write(client->connfd, buf, nbytes) : SSL_write(client->ssl, buf, nbytes));
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

#define WRITE_ERROR_MESSAGE(client, message) _write((client), RDT_ERR_SL message "\r\n", sizeof(message) + 2)

#if defined(__linux__)
typedef struct epoll_event event_t;
#elif defined(__APPLE__)
typedef struct kevent event_t;
#endif
