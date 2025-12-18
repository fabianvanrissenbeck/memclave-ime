/*
* Histogram (HST-L) with multiple tasklets
*
*/
#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <mram.h>
#include <alloc.h>
#include <perfcounter.h>
#include <barrier.h>
#include <atomic_bit.h>
#include <mutex.h>
#include <atomic_bit.h>

#include "support/common.h"
#include "support/log.h"

#define NR_TASKLETS 16
#define NR_HISTO 1
#define ARG_OFFSET  0x2000
#define ARG_SIZE    sizeof(dpu_arguments_t)
#define A_OFFSET    (ARG_OFFSET + ((ARG_SIZE + 0xFF) & ~0xFF))

/* Read args (8B aligned) */
static inline __attribute__((always_inline))
void read_args_aligned(dpu_arguments_t *args) {
    _Alignas(8) uint8_t buf[(sizeof(dpu_arguments_t) + 7u) & ~7u];
    mram_read((__mram_ptr void const*)ARG_OFFSET, buf, sizeof(buf));
    __builtin_memcpy(args, buf, sizeof(dpu_arguments_t));
}

typedef struct { volatile uint32_t v; uint32_t pad; } barrier_slot_t;
__attribute__((aligned(8)))
static struct {
    barrier_slot_t arrive[NR_TASKLETS];
    volatile uint32_t sense;
} gbar;

static inline __attribute__((always_inline)) void mc_fence(void) {
    __asm__ __volatile__("" ::: "memory");
}

static inline void mybarrier_init(void) {
    if (me() == 0) {
        gbar.sense = 0;
        for (uint32_t i = 0; i < NR_TASKLETS; i++) gbar.arrive[i].v = 1;
        mc_fence();
    }
    while (gbar.sense != 0) { mc_fence(); }
}

static inline void mybarrier_wait(void) {
    const uint32_t tid  = me();
    const uint32_t next = !gbar.sense;
    gbar.arrive[tid].v = next;
    mc_fence();

    if (tid == 0) {
        for (uint32_t i = 0; i < NR_TASKLETS; i++)
            while (gbar.arrive[i].v != next) { mc_fence(); }
        gbar.sense = next;
        mc_fence();
    } else {
        while (gbar.sense != next) { mc_fence(); }
    }
}

#define NR_L_TASKLETS (NR_TASKLETS / NR_HISTO)

typedef struct { volatile uint32_t v; uint32_t pad; } gslot_t;
__attribute__((aligned(8)))
static struct {
    gslot_t arrive[NR_HISTO][NR_L_TASKLETS];
    volatile uint32_t sense[NR_HISTO];
} ggrp;

static inline void groupbar_init(void) {
    if (me() == 0) {
        for (uint32_t g = 0; g < NR_HISTO; g++) {
            ggrp.sense[g] = 0;
            for (uint32_t i = 0; i < NR_L_TASKLETS; i++) ggrp.arrive[g][i].v = 1;
        }
        mc_fence();
    }
    /* wait until initialized */
    while (ggrp.sense[0] != 0) { mc_fence(); }
}

static inline void groupbar_wait(uint32_t gid, uint32_t lid) {
    const uint32_t next = !ggrp.sense[gid];
    ggrp.arrive[gid][lid].v = next;
    mc_fence();

    if (lid == 0) {
        for (uint32_t i = 0; i < NR_L_TASKLETS; i++)
            while (ggrp.arrive[gid][i].v != next) { mc_fence(); }
        ggrp.sense[gid] = next;
        mc_fence();
    } else {
        while (ggrp.sense[gid] != next) { mc_fence(); }
    }
}

static uint32_t *message[NR_TASKLETS];    /* only message[0..NR_HISTO-1] used */
mutex_id_t my_mutex[NR_HISTO];           /* one mutex per histogram group */

static void histogram(uint32_t *histo, uint32_t bins, T *input,
                      uint32_t histo_id, uint32_t l_elems) {
    for (uint32_t j = 0; j < l_elems; j++) {
        const uint32_t d = (uint32_t)(((uint32_t)input[j] * bins) >> DEPTH);
        mutex_lock(my_mutex[histo_id]);
        histo[d] += 1;
        mutex_unlock(my_mutex[histo_id]);
    }
}

int main(void) {
    const uint32_t tid = me();

    if (tid == 0) {
        mybarrier_init();
        groupbar_init();
        mem_reset();
        sk_log_init();
    }
    mybarrier_wait();

    dpu_arguments_t args;
    read_args_aligned(&args);

    const uint32_t input_bytes = args.size;
    const uint32_t xfer_bytes  = args.transfer_size;
    const uint32_t bins        = args.bins;

    const uint32_t l_tasklet_id = tid / NR_HISTO;
    const uint32_t my_histo_id  = tid & (NR_HISTO - 1);

    const uint32_t mram_A = (uint32_t)A_OFFSET;
    const uint32_t mram_H = (uint32_t)(A_OFFSET + xfer_bytes);

    /* Scratch block for MRAM reads */
    T *cache_A = (T *)mem_alloc(BLOCK_SIZE);

    /* Allocate one shared histogram per histo-id (by tasklets 0..NR_HISTO-1) */
    if (tid < NR_HISTO) {
        uint32_t *h = (uint32_t *)mem_alloc(bins * sizeof(uint32_t));
        message[tid] = h;
    }

    /* Wait within the group until message[my_histo_id] is ready */
    groupbar_wait(my_histo_id, l_tasklet_id);

    uint32_t *my_histo = message[my_histo_id];

    /* Initialize shared histogram for this group (striped init) */
    for (uint32_t i = l_tasklet_id; i < bins; i += NR_L_TASKLETS) {
        my_histo[i] = 0;
    }
    groupbar_wait(my_histo_id, l_tasklet_id);

    /* Strided scan over MRAM input */
    const uint32_t base = tid << BLOCK_SIZE_LOG2;
    for (uint32_t byte_idx = base; byte_idx < input_bytes; byte_idx += BLOCK_SIZE * NR_TASKLETS) {
        const uint32_t l_bytes = (byte_idx + BLOCK_SIZE >= input_bytes) ? (input_bytes - byte_idx) : BLOCK_SIZE;
        mram_read((__mram_ptr void const *)(mram_A + byte_idx), cache_A, l_bytes);
        histogram(my_histo, bins, cache_A, my_histo_id, l_bytes >> DIV);
    }

    mybarrier_wait();

    /* Merge group histograms into message[0] (same as PRIM) */
    uint32_t *histo_dpu = message[0];
    for (uint32_t b = tid; b < bins; b += NR_TASKLETS) {
        uint32_t acc = 0;
        for (uint32_t g = 0; g < NR_HISTO; g++) acc += message[g][b];
        histo_dpu[b] = acc;
    }

    mybarrier_wait();

    /* Write final histogram to MRAM (tasklet 0) */
    if (tid == 0) {
        uint32_t bytes = bins * sizeof(uint32_t);
        uint32_t off = 0;
        while (off < bytes) {
            const uint32_t chunk = ((bytes - off) > 2048u) ? 2048u : (bytes - off);
            mram_write((void *)((uint8_t *)histo_dpu + off),
                       (__mram_ptr void *)(mram_H + off),
                       chunk);
            off += chunk;
        }
        __ime_wait_for_host();
    }

    return 0;
}
