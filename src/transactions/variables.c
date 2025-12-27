#include <telly.h>

#include <stdbool.h>

#include <semaphore.h>

ThreadQueue *tx_queue = NULL;
sem_t *tx_sem = NULL;
_Atomic bool tx_thread_sleeping;
