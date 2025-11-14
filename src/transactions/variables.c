#include <telly.h>

static struct TransactionVariables variables;

struct TransactionVariables *get_transaction_variables() {
  return &variables;
}
