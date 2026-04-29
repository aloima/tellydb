#include <telly.h>

Server *server = NULL;

#define CLEANUP_RETURN_IF(condition) do { \
  if (condition) { \
    cleanup(); \
    return; \
  } \
} while (0)

#define CLEANUP_RETURN_LOG_IF(condition, fmt, ...) do { \
  if (condition) { \
    write_log(LOG_ERR, (fmt), ##__VA_ARGS__); \
    cleanup(); \
    return; \
  } \
} while (0)

#define LOG_RETURN(value, fmt, ...) do { \
  write_log(LOG_ERR, (fmt), ##__VA_ARGS__); \
  return (value); \
} while (0)

#define LOG_NO_RETURN_VALUE(fmt, ...) do { \
  write_log(LOG_ERR, (fmt), ##__VA_ARGS__); \
  return; \
} while (0)

void get_server_time(time_t *server_start_at, uint32_t *server_age) {
  *server_start_at = server->start_at;
  *server_age = server->age;
}

void terminate_connection(Client *client) {
  const int connfd = client->connfd;
  event_t ev;
  PREPARE_REMOVING_EVENT(ev, connfd);

  if (REMOVE_EVENT(server->eventfd, connfd) == -1)
    LOG_NO_RETURN_VALUE("Cannot remove Client #%" PRIi32 " from multiplexing.", client->id);

  write_log(LOG_DBG, "Client #%" PRIi32 " is disconnected.", client->id);

  close(connfd);
  remove_client(client->id);
}

static inline void cleanup() {
  destroy_transaction_thread();
  usleep(15);

  send_destroy_signal_to_io_threads();
  usleep(15);

  free_transaction_blocks();
  free_commands();
  free_clients();

  if (server->ctx) SSL_CTX_free(server->ctx);

  if (server->sockfd != -1)
    ASSERT(close(server->sockfd), ==, 0);

  if (server->eventfd != -1)
    ASSERT(close(server->eventfd), ==, 0);

  destroy_expiry_set();
  free_constant_passwords();
  free_kdf();
  free_passwords();
  free_databases();
  OPENSSL_cleanup();

  write_log(LOG_INFO, "Free'd all memory blocks and exiting the process...");
  save_and_close_logs();
  free_config(server->conf);

  free(server);
  exit(EXIT_SUCCESS);
}

static inline void close_server() {
  const time_t current_time = time(NULL);
  ASSERT(current_time, !=, INVALID_TIME);

  const uint32_t server_age = server->age + difftime(current_time, server->start_at);
  const clock_t start = clock();

  if (save_data(server_age) == 0) {
    write_log(LOG_INFO, "Saved data in %.3f seconds.", ((float) clock() - start) / CLOCKS_PER_SEC);
  } else {
    write_log(LOG_ERR, "Database cannot be saved to database file, data may be corrupted. Inspect logs for detailed information.");
  }

  if (close_database_fd() == 0) {
    write_log(LOG_INFO, "Closed database file.");
  } else {
    write_log(LOG_ERR, "Database file cannot be closed.");
  }

  cleanup();
}

static void close_signal(int arg) {
  switch (arg) {
    case SIGINT:
      puts("\n-- Received SIGINT signal, closing the server...");
      break;

    case SIGTERM:
      puts("\n-- Received SIGTERM signal, closing the server...");
      break;
  }

  server->closed = true;
}

static int initialize_server_ssl() {
  server->ctx = SSL_CTX_new(TLS_server_method());
  if (!server->ctx) LOG_RETURN(-1, "Cannot open SSL context for server.");

  if (SSL_CTX_use_certificate_file(server->ctx, server->conf->cert, SSL_FILETYPE_PEM) <= 0)
    LOG_RETURN(-1, "Server certificate file is malformed.");

  if (SSL_CTX_use_PrivateKey_file(server->ctx, server->conf->private_key, SSL_FILETYPE_PEM) <= 0)
    LOG_RETURN(-1, "Server private key is malformed.");

  write_log(LOG_INFO, "Created SSL context.");
  return 0;
}

static int initialize_socket() {
  if ((server->sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    LOG_RETURN(-1, "Cannot open socket.");

  const int sockfd = server->sockfd;

  {
    const int flags = fcntl(server->sockfd, F_GETFL, 0);
    ASSERT(flags, !=, -1);

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
      LOG_RETURN(-1, "Cannot set non-blocking socket.");
  }

  { // Flags needs to be defined as independently, it may be change.
    const int flag = 1;
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) == -1)
      LOG_RETURN(-1, "Cannot set no-delay socket.");
  }

  {
    const int flag = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1)
      LOG_RETURN(-1, "Cannot set reusable socket.");
  }

  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(server->conf->port);
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

  if ((bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) != 0)
    LOG_RETURN(-1, "Cannot bind socket and address.");

  if (listen(sockfd, 64) != 0)
    LOG_RETURN(-1, "Cannot listen socket.");

  return 0;
}

static int initialize_authorization() {
  if (!create_constant_passwords()) return -1;
  if (!initialize_kdf()) return -1;

  write_log(LOG_INFO, "Created constant passwords and key deriving algorithm.");
  return 0;
}

void start_server(Config *config) {
  server = calloc(1, sizeof(Server));
  if (server == NULL) {
    write_log(LOG_ERR, "Cannot initialize server, out of memory.");
    return;
  }

  server->conf = config ?: get_config(NULL);
  server->eventfd = -1;
  server->sockfd = -1;
  server->last_error_at = 0;
  server->status = SERVER_STATUS_STARTING;

  if (initialize_logs() == -1)
    write_log(LOG_INFO, "Initialized configuration.");
  else
    write_log(LOG_INFO, "Initialized logs and configuration.");

  const pid_t pid = getpid();
  write_log(LOG_INFO, "version: " VERSION ", commit hash: " GIT_HASH ", process id: %d", pid);

  if (config == NULL)
    write_log(LOG_WARN, "No configuration file. To specify, create .tellyconf or use `telly config /path/to/file`.");

  if (server->conf->tls)
    CLEANUP_RETURN_IF(initialize_server_ssl() == -1);
  else
    server->ctx = NULL;

  server->commands = load_commands();
  if (server->commands == NULL) return;

  write_log(LOG_INFO, "Loaded commands.");

  CLEANUP_RETURN_IF(create_transaction_thread() == -1);
  write_log(LOG_INFO, "Created transaction thread.");

  ASSERT(signal(SIGTERM, close_signal), !=, SIG_ERR);
  ASSERT(signal(SIGINT, close_signal), !=, SIG_ERR);

  CLEANUP_RETURN_IF(initialize_socket() == -1);
  CLEANUP_RETURN_IF(initialize_authorization() == -1);
  CLEANUP_RETURN_IF(open_database_fd(&server->age) == -1);
  CLEANUP_RETURN_LOG_IF(create_expiry_set() == -1, "Cannot create expiry set for key-value pairs.");

  write_log(LOG_INFO, "tellydb server age: %" PRIu32 " seconds", server->age);

  server->eventfd = CREATE_EVENTFD();
  CLEANUP_RETURN_LOG_IF(server->eventfd == -1, "Cannot create epoll instance.");

  event_t event;
  CREATE_EVENT(event, server->sockfd);
  CLEANUP_RETURN_LOG_IF(ADD_EVENT(server->eventfd, server->sockfd, event) == -1, "Cannot create epoll instance.");

  CLEANUP_RETURN_IF(initialize_clients() == -1);

  CLEANUP_RETURN_LOG_IF(create_io_threads() == -1, "Cannot create I/O thread.");
  write_log(LOG_INFO, "Created I/O thread.");

  server->start_at = time(NULL);
  ASSERT(server->start_at, !=, INVALID_TIME);

  server->status = SERVER_STATUS_ONLINE;
  write_log(LOG_INFO, "Server is listening on %" PRIu16 " port for accepting connections...", server->conf->port);

  handle_events();

  server->status = SERVER_STATUS_CLOSED;
  close_server();
}
