#include <telly.h>

#include "../headers/transactions/private.h"

#include <stdint.h>
#include <stdatomic.h>

static _Atomic uint32_t at = 0, end = 0; // Accessed by reserve_transaction_block method from process and thread.
static _Atomic uint32_t waiting_blocks = 0; // Accessed by reserve_transaction_block method from process and thread.

static char *buffer; // Accessed by thread.
static struct TransactionBlock *blocks; // Accessed by process and thread, but atomicity is guaranteed by at/end variables.
static struct Command *commands; // Accessed by process and thread, but no need atomicity, because it will not be change.

static pthread_cond_t cond = PTHREAD_COND_INITIALIZER; // Accessed by process and thread.

struct TransactionVariables get_transaction_variables() {
  struct TransactionVariables variables = {
    .at = &at,
    .end = &end,
    .waiting_blocks = &waiting_blocks,
    .buffer = &buffer,
    .blocks = &blocks,
    .commands = &commands,
    .cond = &cond
  };

  return variables;
}
