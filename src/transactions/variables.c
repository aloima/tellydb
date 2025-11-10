#include <telly.h>

#include <stdint.h>
#include <stdatomic.h>

static struct TransactionVariables variables;

struct TransactionVariables *get_transaction_variables() {
  return &variables;
}
