#include <telly.h>

ThreadQueue *tx_queue = NULL;
event_notifier_t *tx_notifier = NULL;
time_t tx_last_saved_at;
