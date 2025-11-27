#include <telly.h>

#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <inttypes.h>
#include <time.h>

#if defined(__linux__)
  #include <sys/epoll.h>

  #define CREATE_EVENTFD() epoll_create1(0)
#elif defined(__APPLE__)
  #include <sys/event.h>
  #include <sys/time.h>

  #define CREATE_EVENTFD() kqueue()
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>

#include <openssl/ssl.h>
#include <openssl/crypto.h>

static struct Server *server;

struct Configuration *get_server_configuration() {
  return server->conf;
}

void get_server_time(time_t *server_start_at, uint32_t *server_age) {
  *server_start_at = server->start_at;
  *server_age = server->age;
}

void terminate_connection(struct Client *client) {
  const int connfd = client->connfd;

#if defined(__linux__)
  if (epoll_ctl(server->eventfd, EPOLL_CTL_DEL, connfd, NULL) == -1) {
#elif defined(__APPLE__)
  struct kevent ev;
  EV_SET(&ev, connfd, EVFILT_READ, EV_DELETE, 0, 0, NULL);

  if (kevent(server->eventfd, &ev, 1, NULL, 0, NULL) == -1) {
#endif
    write_log(LOG_ERR, "Cannot remove Client #%" PRIi32 " from multiplexing.", client->id);
    return;
  }

  write_log(LOG_DBG, "Client #%" PRIu32 " is disconnected.", client->id);

  if (server->conf->tls) SSL_shutdown(client->ssl);
  close(connfd);
  remove_client(client->id);
}

static inline void cleanup() {
  free_client_maps();
  if (server->ctx) SSL_CTX_free(server->ctx);
  deactive_transaction_thread();
  usleep(15);
  free_commands();
  close(server->sockfd);
  free_constant_passwords();
  free_kdf();
  close(server->eventfd);
  free_passwords();
  free_transactions();
  free_databases();
  destroy_io_threads();

  write_log(LOG_INFO, "Free'd all memory blocks and exiting the process...");
  save_and_close_logs();
  free_configuration(server->conf);

  free(server);
  exit(EXIT_SUCCESS);
}

static inline void close_server() {
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

  cleanup();
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
    return -1;
  }

  if (SSL_CTX_use_certificate_file(server->ctx, server->conf->cert, SSL_FILETYPE_PEM) <= 0) {
    write_log(LOG_ERR, "Server certificate file cannot be used.");
    return -1;
  }

  if (SSL_CTX_use_PrivateKey_file(server->ctx, server->conf->private_key, SSL_FILETYPE_PEM) <= 0 ) {
    write_log(LOG_ERR, "Server private key cannot be used.");
    return -1;
  }

  write_log(LOG_INFO, "Created SSL context.");
  return 0;
}

static int initialize_socket() {
  if ((server->sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    write_log(LOG_ERR, "Cannot open socket, safely exiting...");
    return -1;
  }

  if (fcntl(server->sockfd, F_SETFL, fcntl(server->sockfd, F_GETFL, 0) | O_NONBLOCK) == -1) {
    write_log(LOG_ERR, "Cannot set non-blocking socket, safely exiting...");
    return -1;
  }

  const int flag = 1;

  if (setsockopt(server->sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) == -1) {
    write_log(LOG_ERR, "Cannot set no-delay socket, safely exiting...");
    return -1;
  }

  if (setsockopt(server->sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1) {
    write_log(LOG_ERR, "Cannot set reusable socket, safely exiting...");
    return -1;
  }

  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(server->conf->port);
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

  if ((bind(server->sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) != 0) {
    write_log(LOG_ERR, "Cannot bind socket and address.");
    return -1;
  }

  if (listen(server->sockfd, 64) != 0) {
    write_log(LOG_ERR, "Cannot listen socket.");
    return -1;
  }

  return 0;
}

static int initialize_authorization() {
  if (!create_constant_passwords()) {
    cleanup();
    return -1;
  }

  if (!initialize_kdf()) {
    cleanup();
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

  const pid_t pid = getpid();
  write_log(LOG_INFO, "version: " VERSION ", commit hash: " GIT_HASH ", process id: %d", pid);

  if (server->conf->default_conf) {
    write_log(LOG_WARN, "No configuration file. To specify, create .tellyconf or use `telly config /path/to/file`.");
  }

  if (server->conf->tls) {
    if (initialize_server_ssl() == -1) {
      cleanup();
      return;
    }
  } else {
    server->ctx = NULL;
  }

  server->commands = load_commands();
  write_log(LOG_INFO, "Loaded commands.");

  create_transaction_thread();
  write_log(LOG_INFO, "Created transaction thread.");

  create_io_threads(4);
  write_log(LOG_INFO, "Created I/O thread.");

  signal(SIGTERM, close_signal);
  signal(SIGINT, close_signal);

  if (initialize_socket() == -1) {
    cleanup();
    return;
  }

  if (initialize_authorization() == -1) {
    cleanup();
    return;
  }

  if (!open_database_fd(&server->age)) {
    cleanup();
    return;
  }

  write_log(LOG_INFO, "tellydb server age: %" PRIu32 " seconds", server->age);

  if ((server->eventfd = CREATE_EVENTFD()) == -1) {
		write_log(LOG_ERR, "Cannot create epoll instance.");
    cleanup();
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
    write_log(LOG_ERR, "Cannot add server to event instance.");
    cleanup();
    return;
  }

  if (!initialize_client_maps()) {
    cleanup();
    return;
  }

  server->start_at = time(NULL);
  write_log(LOG_INFO, "Server is listening on %" PRIu16 " port for accepting connections...", server->conf->port);

  handle_events(server);
  close_server();
}
