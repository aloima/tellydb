#include <telly.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int query_server_status(const Config *conf) {
  if (server != NULL) {
    // Server is running in the same process, get status from memory
    time_t start_at;
    uint32_t age;
    get_server_time(&start_at, &age);

    /* Age from server start + runtime since last snapshot */
    age += difftime(time(NULL), start_at);

    char start_at_buf[21];
    generate_date_string(start_at_buf, start_at);

    char last_error_buf[32];
    if (server->last_error_at != 0) {
      generate_date_string(last_error_buf, server->last_error_at);
    } else {
      sprintf(last_error_buf, "none");
    }

    const char *status_text;
    switch (server->status) {
      case SERVER_STATUS_STARTING: status_text = "starting"; break;
      case SERVER_STATUS_ONLINE: status_text = "online"; break;
      case SERVER_STATUS_ERROR: status_text = "error"; break;
      case SERVER_STATUS_CLOSED: status_text = "closed"; break;
      default: status_text = "unknown"; break;
    }

    printf("# Status\n");
    printf("Version: " VERSION "\n");
    printf("Status: %s\n", status_text);
    printf("Started at: %s\n", start_at_buf);
    printf("Age: %" PRIu32 " seconds\n", age);
    printf("Connected clients: %u\n", get_client_count());
    printf("Total processed transactions: %" PRIu64 "\n", get_processed_transaction_count());
    printf("LastErrorDate: %s\n", last_error_buf);

    return 0;
  }

  // Server is not running in this process, connect via socket
  const char *host = "127.0.0.1";
  uint16_t port = conf ? conf->port : 6379;

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    fprintf(stderr, "Error: Cannot create socket (%s)\n", strerror(errno));
    return -1;
  }

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
    fprintf(stderr, "Error: Invalid address %s\n", host);
    close(sock);
    return -1;
  }

  if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
    fprintf(stderr, "Error: Cannot connect to server %s:%u (%s)\n", host, port, strerror(errno));
    close(sock);
    return -1;
  }

  const char *request = "*1\r\n$6\r\nSTATUS\r\n";
  if (send(sock, request, strlen(request), 0) == -1) {
    fprintf(stderr, "Error: Cannot send STATUS command (%s)\n", strerror(errno));
    close(sock);
    return -1;
  }

  char buffer[16384];
  ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
  if (received <= 0) {
    fprintf(stderr, "Error: No response from server (%s)\n", strerror(errno));
    close(sock);
    return -1;
  }
  buffer[received] = '\0';

  if (buffer[0] == '$') {
    char *payload = strstr(buffer, "\r\n");
    if (payload) {
      payload += 2;
      printf("%s", payload);
    } else {
      printf("%s", buffer);
    }
  } else {
    printf("%s", buffer);
  }

  close(sock);
  return 0;
}

int main(int argc, char *argv[]) {
  switch (argc) {
    case 1:
      start_server(NULL);
      return EXIT_SUCCESS;

    case 2: {
      const char *arg = argv[1];

      if (streq(arg, "version")) {
        puts((
          "tellydb version " VERSION "\n"
          "open source at https://github.com/aloima/tellydb\n"
          "licensed under BSD-3 Clause Clear License"
        ));
        return EXIT_SUCCESS;
      } else if (streq(arg, "help")) {
        puts((
          "telly [<ARGUMENT>]\n\n"
          "Arguments:\n"
          " version        - Prints version\n"
          " help           - Prints this page\n"
          " status         - Shows server status by querying STATUS command on 127.0.0.1:6379\n"
          " config [FILE]  - Runs the server using configuration file. If file is not exist, use .tellyconf\n"
          " create-config  - Creates a .tellyconf file using default values. If it is exists, it will be discarded\n"
          " default-config - Prints default configuration values\n\n"
          "Without an argument, starts the server using .tellyconf file in working directory.\n"
          "If .tellyconf is not exists, starts the server using default values."
        ));

        return EXIT_SUCCESS;
      } else if (streq(arg, "status")) {
        Config *conf = get_default_config();
        const int status = query_server_status(conf);
        free_config(conf);
        return status == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
      } else if (streq(arg, "default-config")) {
        Config *conf = get_default_config();
        char buf[1024];

        get_config_string(buf, conf);
        puts(buf);

        return EXIT_SUCCESS;
      } else if (streq(arg, "create-config")) {
        FILE *file = fopen(".tellyconf", "w");
        if (!file) {
          fprintf(stderr, "Error: Cannot create .tellyconf file\n");
          return EXIT_FAILURE;
        }
        Config *conf = get_default_config();
        char buf[4096];

        const size_t n = get_config_string(buf, conf);
        fwrite(buf, sizeof(char), n, file);
        fclose(file);

        return EXIT_SUCCESS;
      } else {
        fputs("Invalid argument, use help command\n", stderr);
        return EXIT_FAILURE;
      }
    }

    case 3:
      if (streq(argv[1], "config")) {
        Config *config = get_config(argv[2]);
        start_server(config);
        return EXIT_SUCCESS;
      } else {
        fputs("Invalid argument usage, use help command\n", stderr);
        return EXIT_FAILURE;
      }

    default:
      fputs("Invalid argument count, use help command\n", stderr);
      return EXIT_FAILURE;
  }
}
