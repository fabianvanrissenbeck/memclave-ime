#include "barrier.h"

#include <defs.h>

/**
 * Enter the barrier by increment the count field atomically.
 * If the count field is already at its max, simply spin on the barrier
 * until count is reset by threads leaving the barrier.
 */
static void ime_enter_barrier(volatile ime_barrier* bar) {
    uint16_t count = 0;
    uint16_t id = me();

    do {
        mutex_lock(bar->lock);
        count = bar->count;

        if (count == bar->init) {
            mutex_unlock(bar->lock);
        } else {
            break;
        }
    } while (1);

    if (id < bar->leader) {
        bar->leader = id;
    }

    bar->count += 1;
    mutex_unlock(bar->lock);
}

/**
 * Wait for all threads to have entered the barrier.
 * This is indicated bycount reaching its maximum.
 */
static void ime_wait_barrier(volatile ime_barrier* bar) {
    bool is_done = false;

    while (!is_done) {
        mutex_lock(bar->lock);
        is_done = bar->count == bar->init;
        mutex_unlock(bar->lock);
    }

    mutex_lock(bar->lock);
    bar->done += 1;
    mutex_unlock(bar->lock);
}

/**
 * All threads except for one don't really do anything here.
 * One thread, the leader, waits for all other threads to have left
 * the ime_wait_barrier function (indicated by done being max). Once
 * this has happened, no other thread of this batch will query the
 * barrier state again, making it safe to reset the count, done and
 * leader values.
 *
 * @returns true if the calling thread is the barrier leader
 */
static bool ime_leave_barrier(volatile ime_barrier* bar) {
    unsigned id = me();
    bool is_leader;

    mutex_lock(bar->lock);
    is_leader = bar->leader == id;
    mutex_unlock(bar->lock);

    if (is_leader) {
        bool is_done = false;

        while (!is_done) {
            mutex_lock(bar->lock);
            is_done = bar->done == bar->init;
            mutex_unlock(bar->lock);
        }

        mutex_lock(bar->lock);
        bar->count = 0;
        bar->done = 0;
        bar->leader = 0xFFFF;
        mutex_unlock(bar->lock);
    }

    return is_leader;
}

bool ime_barrier_wait(volatile ime_barrier* bar) {
    ime_enter_barrier(bar);
    ime_wait_barrier(bar);
    return ime_leave_barrier(bar);
}
