#pragma once

#include "./_client.h"
#include "../config.h"
#include "../utils.h"

#include <stdint.h>
#include <time.h>

#include <sys/types.h>

void terminate_connection(const int connfd);
off_t *get_authorization_end_at();
void get_server_time(time_t *server_start_at, uint32_t *server_age);
void handle_events(struct Configuration *conf, SSL_CTX *ctx, const int sockfd, struct Command *commands, const int eventfd);
void start_server(struct Configuration *config);
struct Configuration *get_server_configuration();

string_t write_value(void *value, const enum TellyTypes type, const enum ProtocolVersion protover, char *buffer);
