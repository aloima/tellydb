#include "../../headers/server.h"

#include <unistd.h>

ssize_t _read(struct Client *client, void *buf, const size_t nbytes) {
  if (client->ssl) {
    return SSL_read(client->ssl, buf, nbytes);
  } else {
    return read(client->connfd, buf, nbytes);
  }
}

ssize_t _write(struct Client *client, void *buf, const size_t nbytes) {
  if (client->ssl) {
    return SSL_write(client->ssl, buf, nbytes);
  } else {
    return write(client->connfd, buf, nbytes);
  }
}
