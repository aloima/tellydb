#include <telly.h>

#include "../headers/transactions/private.h"

#include <stdint.h>
#include <stdatomic.h>

static struct ThreadQueue *queue;
static char *buffer; // Accessed by thread.
static struct Command *commands; // Accessed by process and thread, but no need atomicity, because it will not be change.

static pthread_cond_t cond = PTHREAD_COND_INITIALIZER; // Accessed by process and thread.
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Accessed by process and thread.

struct TransactionVariables get_transaction_variables() {
  struct TransactionVariables variables = {
    .queue = &queue,
    .buffer = &buffer,
    .commands = &commands,
    .cond = &cond,
    .mutex = &mutex
  };

  return variables;
}
