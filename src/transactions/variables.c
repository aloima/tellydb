#include <telly.h>

static TransactionVariables variables;

TransactionVariables *get_transaction_variables() {
  return &variables;
}
