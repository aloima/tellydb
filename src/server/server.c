#include <telly.h>

#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <inttypes.h>
#include <stdatomic.h>
#include <time.h>

#if defined(__linux__)
  #include <sys/epoll.h>

  #define CREATE_EVENTFD() epoll_create1(0)

  #define CREATE_EVENT(event, sockfd) do { \
    (event).events = EPOLLIN; \
    (event).data.fd = (sockfd); \
  } while (0)

  #define ADD_EVENT(eventfd, sockfd, event) epoll_ctl((eventfd), EPOLL_CTL_ADD, (sockfd), &(event))

  #define REMOVE_EVENT(eventfd, connfd) epoll_ctl((eventfd), EPOLL_CTL_DEL, (connfd), NULL)
  #define PREPARE_REMOVING_EVENT(ev, connfd) (void) ev, (void) connfd
#elif defined(__APPLE__)
  #include <sys/event.h>
  #include <sys/time.h>

  #define CREATE_EVENTFD() kqueue()
  #define CREATE_EVENT(event, sockfd) EV_SET(&(event), (sockfd), EVFILT_READ, EV_ADD, 0, 0, NULL)
  #define ADD_EVENT(eventfd, sockfd, event) kevent((eventfd), &(event), 1, NULL, 0, NULL)

  #define REMOVE_EVENT(eventfd, connfd) kevent((eventfd), &ev, 1, NULL, 0, NULL)
  #define PREPARE_REMOVING_EVENT(ev, connfd) EV_SET(&(ev), (connfd), EVFILT_READ, EV_DELETE, 0, 0, NULL)
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>

#include <openssl/ssl.h>
#include <openssl/crypto.h>

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

  destroy_io_threads();
  usleep(10);

  free_transaction_blocks();
  free_commands();
  free_clients();

  if (server->ctx) SSL_CTX_free(server->ctx);
  if (server->sockfd != -1) close(server->sockfd);
  if (server->eventfd != -1) close(server->eventfd);

  free_constant_passwords();
  free_kdf();
  free_passwords();
  free_databases();

  write_log(LOG_INFO, "Free'd all memory blocks and exiting the process...");
  save_and_close_logs();
  free_config(server->conf);

  free(server);
  exit(EXIT_SUCCESS);
}

static inline void close_server() {
  const uint32_t server_age = server->age + difftime(time(NULL), server->start_at);
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
  if ((server->sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) LOG_RETURN(-1, "Cannot open socket.");
  const int sockfd = server->sockfd;

  if (fcntl(sockfd, F_SETFL, fcntl(server->sockfd, F_GETFL, 0) | O_NONBLOCK) == -1) LOG_RETURN(-1, "Cannot set non-blocking socket.");

  { // Flags needs to be defined as independently, it may be change.
    const int flag = 1;
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) == -1) LOG_RETURN(-1, "Cannot set no-delay socket.");
  }

  {
    const int flag = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1) LOG_RETURN(-1, "Cannot set reusable socket.");
  }

  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(server->conf->port);
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

  if ((bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) != 0) LOG_RETURN(-1, "Cannot bind socket and address.");
  if (listen(sockfd, 64) != 0) LOG_RETURN(-1, "Cannot listen socket.");

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
  server->conf = config ?: get_default_config();
  server->eventfd = -1;
  server->sockfd = -1;

  if (!initialize_logs()) {
    write_log(LOG_ERR, "Cannot initialized logs.");
    write_log(LOG_INFO, "Initialized configuration.");
  } else {
    write_log(LOG_INFO, "Initialized logs and configuration.");
  }

  const pid_t pid = getpid();
  write_log(LOG_INFO, "version: " VERSION ", commit hash: " GIT_HASH ", process id: %d", pid);

  if (config == NULL) {
    write_log(LOG_WARN, "No configuration file. To specify, create .tellyconf or use `telly config /path/to/file`.");
  }

  if (server->conf->tls) CLEANUP_RETURN_IF(initialize_server_ssl() == -1);
  else server->ctx = NULL;

  server->commands = load_commands();
  write_log(LOG_INFO, "Loaded commands.");

  create_transaction_thread();
  write_log(LOG_INFO, "Created transaction thread.");

  const uint32_t thread_count = max(sysconf(_SC_NPROCESSORS_ONLN) - 1, 1);
  CLEANUP_RETURN_LOG_IF(create_io_threads(thread_count) == -1, "Cannot create I/O threads.");
  write_log(LOG_INFO, "Created I/O thread.");

  signal(SIGTERM, close_signal);
  signal(SIGINT, close_signal);

  CLEANUP_RETURN_IF(initialize_socket() == -1);
  CLEANUP_RETURN_IF(initialize_authorization() == -1);
  CLEANUP_RETURN_IF(open_database_fd(&server->age) == -1);

  write_log(LOG_INFO, "tellydb server age: %" PRIu32 " seconds", server->age);

  server->eventfd = CREATE_EVENTFD();
  CLEANUP_RETURN_LOG_IF(server->eventfd == -1, "Cannot create epoll instance.");

  event_t event;
  CREATE_EVENT(event, server->sockfd);
  CLEANUP_RETURN_LOG_IF(ADD_EVENT(server->eventfd, server->sockfd, event) == -1, "Cannot create epoll instance.");

  CLEANUP_RETURN_IF(initialize_clients() == -1);

  server->start_at = time(NULL);
  write_log(LOG_INFO, "Server is listening on %" PRIu16 " port for accepting connections...", server->conf->port);

  handle_events();
  close_server();
}
