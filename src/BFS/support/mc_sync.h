/* support/mc_sync.h
 * Minimal sync primitives for Memclave subkernels.
 *   void mybarrier_init(void);
 *   void mybarrier_wait(void);
 *   void handshake_init(void);
 *   T    handshake_sync(T l_count, unsigned tasklet_id, T *next_partial_accum);
 *
 * Usage notes:
 *  - Requires NR_TASKLETS to be defined by the including TU.
 *  - Include after support/common.h so typedef T is visible.
 */

#ifndef MC_SYNC_H
#define MC_SYNC_H

#include <stdint.h>
#include <defs.h>
#include <alloc.h>

#ifndef NR_TASKLETS
# error "NR_TASKLETS must be defined before including mc_sync.h"
#endif

/* -------- Memory fence helper -------- */
static inline __attribute__((always_inline)) void mc_fence(void) {
    __asm__ __volatile__("" ::: "memory");
}

/* -------- Sense-reversing barrier -------- */
typedef struct { volatile uint32_t v; uint32_t _pad; } mc_bar_slot_t;

__attribute__((aligned(8)))
static struct {
    mc_bar_slot_t arrive[NR_TASKLETS];
    volatile uint32_t sense;
} mc_barrier;

static inline __attribute__((always_inline)) void mybarrier_init(void) {
    if (me() == 0) {
        mc_barrier.sense = 0;
        for (uint32_t i = 0; i < NR_TASKLETS; i++) mc_barrier.arrive[i].v = 1;
        mc_fence();
    }
    while (mc_barrier.sense != 0) { mc_fence(); }
}

static inline __attribute__((always_inline)) void mybarrier_wait(void) {
    const uint32_t tid   = me();
    const uint32_t nextS = !mc_barrier.sense;
    mc_barrier.arrive[tid].v = nextS;
    mc_fence();
    if (tid == 0) {
        for (uint32_t i = 0; i < NR_TASKLETS; i++)
            while (mc_barrier.arrive[i].v != nextS) { mc_fence(); }
        mc_barrier.sense = nextS;
        mc_fence();
    } else {
        while (mc_barrier.sense != nextS) { mc_fence(); }
    }
}

/* -------- Handshake -------- */
/* Shared message slots (value + optional epoch); aligned for WRAM access. */
typedef struct {
    volatile T        val;
    volatile uint32_t epoch;
} mc_msg_slot_t;

__attribute__((aligned(8))) static mc_msg_slot_t mc_msg[NR_TASKLETS];
__attribute__((aligned(8))) static volatile uint32_t g_epoch;

/* Initialize handshake state (works for both implementations). */
static inline __attribute__((always_inline)) void handshake_init(void) {
    if (me() == 0) {
        g_epoch = 1;
        for (uint32_t i = 0; i < NR_TASKLETS; i++) {
            mc_msg[i].val   = (T)0;
            mc_msg[i].epoch = 0;
        }
        mc_fence();
    }
}

/* --- Variant A: Mailbox + epoch (default) --- */
#ifndef USE_UPMEM_HANDSHAKE

/* Caller contract:
 *  - Bump g_epoch once per round (by tasklet 0), then mybarrier_wait().
 *  - Then call handshake_sync() on each tasklet to propagate partials.
 */
static inline __attribute__((always_inline))
T handshake_sync(T l_count, unsigned tasklet_id, T *next_partial_accum /*out*/) {
    const uint32_t epoch = (uint32_t)g_epoch;

    T p_count = (T)0;
    if (tasklet_id != 0) {
        while (mc_msg[tasklet_id].epoch != epoch) { mc_fence(); }
        p_count = mc_msg[tasklet_id].val;
    }

    if (tasklet_id < (NR_TASKLETS - 1)) {
        mc_msg[tasklet_id + 1].val   = p_count + l_count;
        mc_fence();
        mc_msg[tasklet_id + 1].epoch = epoch;
        mc_fence();
    } else if (next_partial_accum) {
        *next_partial_accum = p_count + l_count;
    }

    return p_count;
}

#else  /* USE_UPMEM_HANDSHAKE */

#include <handshake.h>
#ifndef MC_TASKLET_SYSNAME
# error "Define MC_TASKLET_SYSNAME(id) to map tasklet index to a sysname_t notifier."
#endif

static inline __attribute__((always_inline))
T handshake_sync(T l_count, unsigned tasklet_id, T *next_partial_accum /*out*/) {
    T p_count = (T)0;

    if (tasklet_id != 0) {
        /* Wait for previous tasklet to notify */
        (void)handshake_wait_for(MC_TASKLET_SYSNAME(tasklet_id - 1));
        p_count = mc_msg[tasklet_id].val;
    }

    if (tasklet_id < (NR_TASKLETS - 1)) {
        mc_msg[tasklet_id + 1].val = p_count + l_count;
        mc_fence();
        /* Notify next tasklet */
        handshake_notify();
    } else if (next_partial_accum) {
        *next_partial_accum = p_count + l_count;
    }

    return p_count;
}
#endif /* USE_UPMEM_HANDSHAKE */

#endif /* MC_SYNC_H */
