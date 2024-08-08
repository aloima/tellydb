#include "../../headers/telly.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

void start_server(struct Configuration conf) {
  load_commands();

  int sockfd;
  struct sockaddr_in servaddr, addr;

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    fputs("cannot open socket\n", stderr);
    return;
  }

  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(conf.port);

  if ((bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) != 0) { 
    fputs("cannot bind socket\n", stderr);
    return;
  }

  if (listen(sockfd, 10) != 0) { 
    fputs("cannot listen socket\n", stderr);
    return;
  }

  socklen_t addr_len = sizeof(addr);
  int connfd = accept(sockfd, (struct sockaddr *) &addr, &addr_len);

  while (true) {
    respdata_t data = get_resp_data(connfd);
    execute_commands(connfd, data);
  }

  close(sockfd);
}
