#ifndef IME_BARRIER_H
#define IME_BARRIER_H

#include <mutex.h>

/**
 * @brief create a new barrier called <name> - barriers are always global and use one mutex
 * @param name global name of the barrier
 * @param c number of threads to wait for using this barrier
 */
#define IME_BARRIER_INIT(name, c) \
    MUTEX_INIT(bar_lock_ ##name); \
    volatile ime_barrier name = { \
        .lock = MUTEX_GET(bar_lock_ ##name), \
        .init = c, \
        .count = 0, \
        .done = 0, \
        .leader = 0xFFFF, \
    };

typedef struct ime_barrier {
    mutex_id_t lock;
    const uint16_t init; // number of threads for the barrier to open
    uint16_t count; // number of threads arrived at the barrier
    uint16_t done; // number of threads done waiting
    uint16_t leader; // id of the leader thread (one of the waiting threads)
} ime_barrier;

/**
 * @brief block until bar->init many threads have called this function
 * @param bar barrier to block on
 * @return true for exactly one of the calling threads and false otherwise
 */
bool ime_barrier_wait(volatile ime_barrier* bar);

#endif