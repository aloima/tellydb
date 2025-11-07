#include <telly.h>

#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <inttypes.h>
#include <time.h>

#if defined(__linux__)
#include <sys/epoll.h>
#elif defined(__APPLE__)
#include <sys/event.h>
#include <sys/time.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>

#include <openssl/ssl.h>
#include <openssl/crypto.h>

#define FREE_CONF_LOGS(server) {\
  save_and_close_logs();\
  free_configuration(server->conf);\
}

#define FREE_CTX_THREAD_CMD(server) {\
  if (server->ctx) SSL_CTX_free((server)->ctx);\
  deactive_transaction_thread();\
  usleep(15);\
  free_commands();\
}

#define FREE_CTX_THREAD_CMD_SOCKET(server) {\
  FREE_CTX_THREAD_CMD(server);\
  close((server)->sockfd);\
}

#define FREE_CTX_THREAD_CMD_SOCKET_PASS(server) {\
  FREE_CTX_THREAD_CMD_SOCKET(server);\
  free_constant_passwords();\
}

#define FREE_CTX_THREAD_CMD_SOCKET_PASS_KDF(server) {\
  FREE_CTX_THREAD_CMD_SOCKET_PASS(server);\
  free_kdf();\
}

static struct Server *server;

struct Configuration *get_server_configuration() {
  return server->conf;
}

void get_server_time(time_t *server_start_at, uint32_t *server_age) {
  *server_start_at = server->start_at;
  *server_age = server->age;
}

void terminate_connection(const int connfd) {
  const struct Client *client = get_client(connfd);

#if defined(__linux__)
  if (epoll_ctl(server->eventfd, EPOLL_CTL_DEL, connfd, NULL) == -1) {
#elif defined(__APPLE__)
  struct kevent ev;
  EV_SET(&ev, connfd, EVFILT_READ, EV_DELETE, 0, 0, NULL);

  if (kevent(server->eventfd, &ev, 1, NULL, 0, NULL) == -1) {
#endif
    write_log(LOG_ERR, "Cannot remove Client #%" PRIu32 " from multiplexing.", client->id);
    return;
  }

  write_log(LOG_INFO, "Client #%" PRIu32 " is disconnected.", client->id);

  if (server->conf->tls) {
    SSL_shutdown(client->ssl);
  }

  close(connfd);
  remove_client(connfd);
}

static void close_server() {
  free_client_maps();
  write_log(LOG_INFO, "Terminated all clients.");

  const uint32_t server_age = server->age + difftime(time(NULL), server->start_at);
  const clock_t start = clock();

  if (save_data(server_age)) {
    write_log(LOG_INFO, "Saved data in %.3f seconds.", ((float) clock() - start) / CLOCKS_PER_SEC);
  } else {
    write_log(LOG_ERR, "Database cannot be saved to database file, data may be corrupted. Inspect logs for detailed information.");
  }

  if (close_database_fd()) {
    write_log(LOG_INFO, "Closed database file.");
  } else {
    write_log(LOG_ERR, "Database file cannot be closed.");
  }

  FREE_CTX_THREAD_CMD_SOCKET_PASS_KDF(server);
  close(server->eventfd);
  free_passwords();
  free_transactions();
  free_databases();
  write_log(LOG_INFO, "Free'd all memory blocks and closed the server.");

  write_log(LOG_INFO, "Closing log file, free'ing configuration and exiting the process...");
  FREE_CONF_LOGS(server);

  free(server);
  exit(EXIT_SUCCESS);
}

static void close_signal(int arg) {
  switch (arg) {
    case SIGINT:
      write_log(LOG_WARN, "Received SIGINT signal, closing the server...");
      break;

    case SIGTERM:
      write_log(LOG_WARN, "Received SIGTERM signal, closing the server...");
      break;
  }

  server->closed = true;
}

static int initialize_server_ssl() {
  server->ctx = SSL_CTX_new(TLS_server_method());

  if (!server->ctx) {
    write_log(LOG_ERR, "Cannot open SSL context, safely exiting...");
    FREE_CONF_LOGS(server);
    free(server);
    return -1;
  }

  if (SSL_CTX_use_certificate_file(server->ctx, server->conf->cert, SSL_FILETYPE_PEM) <= 0) {
    write_log(LOG_ERR, "Server certificate file cannot be used.");
    SSL_CTX_free(server->ctx);
    FREE_CONF_LOGS(server);
    free(server);
    return -1;
  }

  if (SSL_CTX_use_PrivateKey_file(server->ctx, server->conf->private_key, SSL_FILETYPE_PEM) <= 0 ) {
    write_log(LOG_ERR, "Server private key cannot be used.");
    SSL_CTX_free(server->ctx);
    FREE_CONF_LOGS(server);
    free(server);
    return -1;
  }

  write_log(LOG_INFO, "Created SSL context.");
  return 0;
}

static int initialize_socket() {
  if ((server->sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    FREE_CTX_THREAD_CMD(server);
    write_log(LOG_ERR, "Cannot open socket, safely exiting...");
    FREE_CONF_LOGS(server);
    free(server);
    return -1;
  }

  if (fcntl(server->sockfd, F_SETFL, fcntl(server->sockfd, F_GETFL, 0) | O_NONBLOCK) == -1) {
    FREE_CTX_THREAD_CMD_SOCKET(server);
    write_log(LOG_ERR, "Cannot set non-blocking socket, safely exiting...");
    FREE_CONF_LOGS(server);
    free(server);
    return -1;
  }

  const int flag = 1;

  if (setsockopt(server->sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) == -1) {
    FREE_CTX_THREAD_CMD_SOCKET(server);
    write_log(LOG_ERR, "Cannot set no-delay socket, safely exiting...");
    FREE_CONF_LOGS(server);
    free(server);
    return -1;
  }

  if (setsockopt(server->sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1) {
    FREE_CTX_THREAD_CMD_SOCKET(server);
    write_log(LOG_ERR, "Cannot set reusable socket, safely exiting...");
    FREE_CONF_LOGS(server);
    free(server);
    return -1;
  }

  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(server->conf->port);
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

  if ((bind(server->sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) != 0) {
    FREE_CTX_THREAD_CMD_SOCKET(server);
    write_log(LOG_ERR, "Cannot bind socket and address.");
    FREE_CONF_LOGS(server);
    free(server);
    return -1;
  }

  if (listen(server->sockfd, server->conf->max_clients) != 0) {
    FREE_CTX_THREAD_CMD_SOCKET(server);
    write_log(LOG_ERR, "Cannot listen socket.");
    FREE_CONF_LOGS(server);
    free(server);
    return -1;
  }

  return 0;
}

static int initialize_authorization() {
  if (!create_constant_passwords()) {
    FREE_CTX_THREAD_CMD_SOCKET(server);
    FREE_CONF_LOGS(server);
    free(server);
    return -1;
  }

  if (!initialize_kdf()) {
    FREE_CTX_THREAD_CMD_SOCKET_PASS(server);
    FREE_CONF_LOGS(server);
    free(server);
    return -1;
  }

  write_log(LOG_INFO, "Created constant passwords and key deriving algorithm.");
  return 0;
}

void start_server(struct Configuration *config) {
  server = malloc(sizeof(struct Server));
  server->conf = config;

  if (!initialize_logs()) {
    write_log(LOG_ERR, "Cannot initialized logs.");
    write_log(LOG_INFO, "Initialized configuration.");
  } else {
    write_log(LOG_INFO, "Initialized logs and configuration.");
  }

  write_log(LOG_INFO, "version=" VERSION ", commit hash=" GIT_HASH);

  if (server->conf->default_conf) {
    write_log(LOG_WARN, "No configuration file. To specify, create .tellyconf or use `telly config /path/to/file`.");
  }

  if (server->conf->tls && (initialize_server_ssl() == -1)) {
    free(server);
    return;
  }

  server->commands = load_commands();
  write_log(LOG_INFO, "Loaded commands.");

  create_transaction_thread();
  write_log(LOG_INFO, "Created transaction thread.");

  signal(SIGTERM, close_signal);
  signal(SIGINT, close_signal);

  if (initialize_socket() == -1) {
    return;
  }

  if (initialize_authorization() == -1) {
    return;
  }

  if (!open_database_fd(&server->age)) {
    FREE_CTX_THREAD_CMD_SOCKET_PASS_KDF(server);
    FREE_CONF_LOGS(server);
    free(server);
    return;
  }

  write_log(LOG_INFO, "tellydb server age: %" PRIu32 " seconds", server->age);

#if defined(__linux__)
  if ((server->eventfd = epoll_create1(0)) == -1) {
#elif defined(__APPLE__)
  if ((server->eventfd = kqueue()) == -1) {
#endif
    FREE_CTX_THREAD_CMD_SOCKET_PASS_KDF(server);
    close_database_fd();
		write_log(LOG_ERR, "Cannot create epoll instance.");
    FREE_CONF_LOGS(server);
    free(server);
    return;
  }

  event_t event;

#if defined(__linux__)
  event.events = EPOLLIN;
  event.data.fd = server->sockfd;

  if (epoll_ctl(server->eventfd, EPOLL_CTL_ADD, server->sockfd, &event) == -1) {
#elif defined(__APPLE__)
  EV_SET(&event, server->sockfd, EVFILT_READ, EV_ADD, 0, 0, NULL);

  if (kevent(server->eventfd, &event, 1, NULL, 0, NULL) == -1) {
#endif
    FREE_CTX_THREAD_CMD_SOCKET_PASS_KDF(server);
    close_database_fd();
    close(server->eventfd);
    write_log(LOG_ERR, "Cannot add server to event instance.");
    FREE_CONF_LOGS(server);
    free(server);
    return;
  }

  if (!initialize_client_maps()) {
    FREE_CTX_THREAD_CMD_SOCKET_PASS_KDF(server);
    close_database_fd();
    close(server->eventfd);
    FREE_CONF_LOGS(server);
    free(server);
    return;
  }

  server->start_at = time(NULL);
  write_log(LOG_INFO, "Server is listening on %" PRIu16 " port for accepting connections...", server->conf->port);

  handle_events(server);
  close_server();
}
