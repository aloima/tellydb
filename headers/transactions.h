#pragma once

#include "database/database.h"
#include "server/server.h"
#include "config.h"
#include "resp.h"
#include "auth.h"

#include <stdbool.h>
#include <stdint.h>

struct Transaction {
  struct Client *client;
  commanddata_t data;
  struct Command *command;
  struct Password *password;
  struct Database *database;
};

void create_transaction_thread(struct Configuration *config);
void deactive_transaction_thread();

uint64_t get_processed_transaction_count();
uint32_t get_transaction_count();
bool add_transaction(struct Client *client, struct Command *command, commanddata_t data);
void remove_transaction(struct Transaction *transaction);
void free_transactions();

void execute_command(struct Transaction *transaction);
