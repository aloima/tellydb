#include <telly.h>

#include <stddef.h>
#include <stdbool.h>
#include <time.h>

#include <semaphore.h>

ThreadQueue *tx_queue = NULL;
sem_t *tx_sem = NULL;
_Atomic(bool) tx_thread_sleeping;
time_t tx_last_saved_at;
