// -----------------------------------------------------------------------------
// main.c  (DPU) — compatible with the host layout above
// -----------------------------------------------------------------------------

/**
* Needleman-Wunsch with multiple tasklets
*
*/
#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <mram.h>
#include <alloc.h>
#include <perfcounter.h>
#include <barrier.h>

#define NR_TASKLETS 16
#define ARG_OFFSET  0x4000u
#define DBG_OFFSET  (ARG_OFFSET + 0x40u)
#define HEAP_BASE   (ARG_OFFSET + 0x100u)   // must match host

#define BL 16
#define BL_IN 4

#include "support/common.h"
#include "support/log.h"
#include "support/mc_sync.h"

typedef struct {
    uint32_t magic;
    uint32_t nblocks;
    uint32_t active_blocks;
    uint32_t penalty;
} dbg_t;

// main
int main() {
    unsigned int tasklet_id = me();

    if (tasklet_id == 0) {
        mybarrier_init();
        mem_reset();
        sk_log_init();
    }
    mybarrier_wait();

    // Layout sanity (build-time)
    _Static_assert((ARG_OFFSET & 7u) == 0, "ARG_OFFSET must be 8B aligned");
    _Static_assert((DBG_OFFSET & 7u) == 0, "DBG_OFFSET must be 8B aligned");
    _Static_assert((HEAP_BASE  & 7u) == 0, "HEAP_BASE must be 8B aligned");
    _Static_assert(sizeof(dpu_arguments_t) <= 0x40, "dpu_arguments_t must fit before DBG_OFFSET");
    _Static_assert((sizeof(dpu_arguments_t) & 7u) == 0, "dpu_arguments_t must be multiple of 8B");

    // Load args from MRAM
    dpu_arguments_t *args = (dpu_arguments_t *)mem_alloc(sizeof(dpu_arguments_t));
    mram_read((__mram_ptr void const *)ARG_OFFSET, args, sizeof(dpu_arguments_t));

    uint32_t nblocks        = args->nblocks;
    uint32_t active_blocks  = args->active_blocks;
    uint32_t penalty        = args->penalty;

    // Nothing to do for inactive DPUs
    if (nblocks == 0) {
        mybarrier_wait();
        return 0;
    }

    // MRAM layout:
    // - input_itemsets region is sized by active_blocks (CEIL layout count)
    // - reference region begins after that
    uint32_t mram_base_addr_input_itemsets = (uint32_t)(HEAP_BASE);

    uint32_t mram_base_addr_ref =
        (uint32_t)(HEAP_BASE + nblocks * (BL + 1) * (BL + 2) * sizeof(int32_t));
    if (nblocks != active_blocks) {
        mram_base_addr_ref =
            (uint32_t)(HEAP_BASE + active_blocks * (BL + 1) * (BL + 2) * sizeof(int32_t));
    }

    // Debug: write back what we saw (tasklet 0 only)
    dbg_t dbg = {
        .magic         = 0x4E573136, // "NW16"
        .nblocks       = args->nblocks,
        .active_blocks = args->active_blocks,
        .penalty       = args->penalty,
    };

    if (tasklet_id == 0) {
        mram_write(&dbg, (__mram_ptr void *)DBG_OFFSET, sizeof(dbg));
    }
    mybarrier_wait();

    int32_t *cache_input = mem_alloc((BL_IN + 1) * (BL_IN + 2) * sizeof(int32_t));
    int32_t *cache_ref   = mem_alloc(BL_IN * BL_IN * sizeof(int32_t));

    uint32_t REP = BL / BL_IN;
    uint32_t chunks;
    uint32_t mod;
    uint32_t start;
    uint32_t addr_input;
    uint32_t addr_ref;
    uint32_t cache_input_offset;

    for (uint32_t bl = 0; bl < nblocks; bl++) {

        // Top-left computation
        for (uint32_t blk = 0; blk <= REP; blk++) {

            // Partition chunks/subblocks of the diagonal to tasklets
            chunks = blk / NR_TASKLETS;
            mod = blk % NR_TASKLETS;

            if (tasklet_id < mod)
                chunks++;

            if (mod > 0) {
                if (tasklet_id < mod)
                    start = tasklet_id * chunks;
                else
                    start = mod * (chunks + 1) + (tasklet_id - mod) * chunks;
            } else {
                start = tasklet_id * chunks;
            }

            // Compute all assigned chunks
            for (uint32_t bl_indx = 0; bl_indx < chunks; bl_indx++) {
                int t_index_x = (int)(start + bl_indx);
                int t_index_y = (int)(blk - 1 - t_index_x);

                // Move input from MRAM to WRAM
                addr_input = mram_base_addr_input_itemsets
                           + (uint32_t)(t_index_x * (BL + 2) * BL_IN * sizeof(int32_t))
                           + (uint32_t)(t_index_y * BL_IN * sizeof(int32_t));

                cache_input_offset = (BL_IN + 2);
                mram_read((__mram_ptr void const *)addr_input, (void *)cache_input, (BL_IN + 2) * sizeof(int32_t));
                addr_input += (uint32_t)((BL + 2) * sizeof(int32_t));

                for (int i = 1; i < (int)BL_IN + 1; i++) {
                    mram_read((__mram_ptr void const *)addr_input,
                              (void *)(cache_input + cache_input_offset),
                              2 * sizeof(int32_t));
                    cache_input_offset += (BL_IN + 2);
                    addr_input += (uint32_t)((BL + 2) * sizeof(int32_t));
                }

                addr_ref = mram_base_addr_ref
                         + (uint32_t)(t_index_x * BL * BL_IN * sizeof(int32_t))
                         + (uint32_t)(t_index_y * BL_IN * sizeof(int32_t));

                cache_input_offset = 0;
                for (int i = 0; i < (int)BL_IN; i++) {
                    mram_read((__mram_ptr void const *)addr_ref,
                              (void *)(cache_ref + cache_input_offset),
                              BL_IN * sizeof(int32_t));
                    cache_input_offset += BL_IN;
                    addr_ref += (uint32_t)(BL * sizeof(int32_t));
                }

                // Computation
                for (uint32_t i = 1; i < BL_IN + 1; i++) {
                    for (uint32_t j = 1; j < BL_IN + 1; j++) {
                        cache_input[i * (BL_IN + 2) + j] =
                            maximum(cache_input[(i - 1) * (BL_IN + 2) + j - 1] + cache_ref[(i - 1) * BL_IN + j - 1],
                                    cache_input[i * (BL_IN + 2) + j - 1] - penalty,
                                    cache_input[(i - 1) * (BL_IN + 2) + j] - penalty);
                    }
                }

                // Move output from WRAM to MRAM
                addr_input = mram_base_addr_input_itemsets
                           + (uint32_t)(t_index_x * (BL + 2) * BL_IN * sizeof(int32_t))
                           + (uint32_t)(t_index_y * BL_IN * sizeof(int32_t));

                cache_input_offset = (BL_IN + 2);
                addr_input += (uint32_t)((BL + 2) * sizeof(int32_t));

                for (int i = 1; i < (int)BL_IN + 1; i++) {
                    mram_write((cache_input + cache_input_offset),
                               (__mram_ptr void *)addr_input,
                               (BL_IN + 2) * sizeof(int32_t));
                    cache_input_offset += (BL_IN + 2);
                    addr_input += (uint32_t)((BL + 2) * sizeof(int32_t));
                }
            }

            mybarrier_wait();
        }

        // Bottom-right computation
        for (uint32_t blk = 2; blk <= REP; blk++) {

            chunks = (REP - blk + 1) / NR_TASKLETS;
            mod = (REP - blk + 1) % NR_TASKLETS;

            if (tasklet_id < mod)
                chunks++;

            if (mod > 0) {
                if (tasklet_id < mod)
                    start = tasklet_id * chunks;
                else
                    start = mod * (chunks + 1) + (tasklet_id - mod) * chunks;
            } else {
                start = tasklet_id * chunks;
            }

            for (uint32_t bl_indx = 0; bl_indx < chunks; bl_indx++) {
                int t_index_x = (int)(blk - 1 + start + bl_indx);
                int t_index_y = (int)(REP + blk - 2 - t_index_x);

                // Move input from MRAM to WRAM
                addr_input = mram_base_addr_input_itemsets
                           + (uint32_t)(t_index_x * (BL + 2) * BL_IN * sizeof(int32_t))
                           + (uint32_t)(t_index_y * BL_IN * sizeof(int32_t));

                cache_input_offset = (BL_IN + 2);
                mram_read((__mram_ptr void const *)addr_input, (void *)cache_input, (BL_IN + 2) * sizeof(int32_t));
                addr_input += (uint32_t)((BL + 2) * sizeof(int32_t));

                for (int i = 1; i < (int)BL_IN + 1; i++) {
                    mram_read((__mram_ptr void const *)addr_input,
                              (void *)(cache_input + cache_input_offset),
                              2 * sizeof(int32_t));
                    cache_input_offset += (BL_IN + 2);
                    addr_input += (uint32_t)((BL + 2) * sizeof(int32_t));
                }

                addr_ref = mram_base_addr_ref
                         + (uint32_t)(t_index_x * BL * BL_IN * sizeof(int32_t))
                         + (uint32_t)(t_index_y * BL_IN * sizeof(int32_t));

                cache_input_offset = 0;
                for (int i = 0; i < (int)BL_IN; i++) {
                    mram_read((__mram_ptr void const *)addr_ref,
                              (void *)(cache_ref + cache_input_offset),
                              BL_IN * sizeof(int32_t));
                    cache_input_offset += BL_IN;
                    addr_ref += (uint32_t)(BL * sizeof(int32_t));
                }

                // Computation
                for (int i = 1; i < (int)BL_IN + 1; i++) {
                    for (int j = 1; j < (int)BL_IN + 1; j++) {
                        cache_input[i * (BL_IN + 2) + j] =
                            maximum(cache_input[(i - 1) * (BL_IN + 2) + j - 1] + cache_ref[(i - 1) * BL_IN + j - 1],
                                    cache_input[i * (BL_IN + 2) + j - 1] - penalty,
                                    cache_input[(i - 1) * (BL_IN + 2) + j] - penalty);
                    }
                }

                // Move output from WRAM to MRAM
                addr_input = mram_base_addr_input_itemsets
                           + (uint32_t)(t_index_x * (BL + 2) * BL_IN * sizeof(int32_t))
                           + (uint32_t)(t_index_y * BL_IN * sizeof(int32_t));

                cache_input_offset = (BL_IN + 2);
                addr_input += (uint32_t)((BL + 2) * sizeof(int32_t));

                for (int i = 1; i < (int)BL_IN + 1; i++) {
                    mram_write(cache_input + cache_input_offset,
                               (__mram_ptr void *)addr_input,
                               (BL_IN + 2) * sizeof(int32_t));
                    cache_input_offset += (BL_IN + 2);
                    addr_input += (uint32_t)((BL + 2) * sizeof(int32_t));
                }
            }

            mybarrier_wait();
        }

        mram_base_addr_input_itemsets += (uint32_t)((BL + 1) * (BL + 2) * sizeof(int32_t));
        mram_base_addr_ref += (uint32_t)(BL * BL * sizeof(int32_t));
    }

    mybarrier_wait();
    return 0;
}

