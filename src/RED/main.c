/*
* Reduction with multiple tasklets
*
*/
#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <mram.h>
#include <alloc.h>
#include <perfcounter.h>
#include <handshake.h>
#include <barrier.h>

#define NR_TASKLETS 16

#include "support/common.h"
#include "support/cyclecount.h"
#include "support/log.h"
#include "support/mc_sync.h"

#define ARG_OFFSET  0x2000
#define ARG_SIZE    ((uint32_t)sizeof(dpu_arguments_t))
#define A_OFFSET    (ARG_OFFSET + ((ARG_SIZE + 0xFF) & ~0xFF))

static inline __attribute__((always_inline))
void read_args_aligned(dpu_arguments_t *args) {
    _Alignas(8) uint8_t buf[(sizeof(dpu_arguments_t) + 7u) & ~7u];
    mram_read((__mram_ptr void const*)ARG_OFFSET, buf, sizeof(buf));
    __builtin_memcpy(args, buf, sizeof(dpu_arguments_t));
}

// Array for communication between adjacent tasklets
T message[NR_TASKLETS];

// Reduction in each tasklet
static T reduction(T *input, unsigned int l_size){
    T output = 0;
    for (unsigned int j = 0; j < l_size; j++){
        output += input[j];
    }
    return output;
}

int main(void) { 
    unsigned int tasklet_id = me();
#if PRINT
    printf("tasklet_id = %u\n", tasklet_id);
#endif
    if (tasklet_id == 0) {
        mybarrier_init();
        mem_reset();
        sk_log_init();
#if PERF
        perfcounter_config(COUNT_CYCLES, true);
#endif
    }
    // Barrier
    mybarrier_wait();

#if PERF && !PERF_SYNC
    result->cycles = 0;
    perfcounter_cycles cycles;
    timer_start(&cycles); // START TIMER
#endif

    dpu_arguments_t args;
    read_args_aligned(&args);
    const uint32_t input_size_bytes = args.size;

    // Address of the current processing block in MRAM
    const uint32_t A_base       = (uint32_t)A_OFFSET;
    uint32_t base_tasklet = tasklet_id << BLOCK_SIZE_LOG2;

    // Initialize a local cache to store the MRAM block
    T *cache_A = (T *) mem_alloc(BLOCK_SIZE);
	
    // Local count
    T l_count = 0;

#if !PERF_SYNC // COMMENT OUT TO COMPARE SYNC PRIMITIVES (Experiment in Appendix)
    for(unsigned int byte_index = base_tasklet; byte_index < input_size_bytes; byte_index += BLOCK_SIZE * NR_TASKLETS){

        // Bound checking
        uint32_t l_size_bytes = (byte_index + BLOCK_SIZE >= input_size_bytes) ? (input_size_bytes - byte_index) : BLOCK_SIZE;

        // Load cache with current MRAM block
        mram_read((__mram_ptr void const*)(A_base + byte_index), cache_A, l_size_bytes);
		
        // Reduction in each tasklet
        l_count += reduction(cache_A, l_size_bytes >> DIV);

    }
#endif

    // Reduce local counts
    message[tasklet_id] = l_count;

#if PERF && PERF_SYNC // TIMER FOR SYNC PRIMITIVES
    result->cycles = 0;
    perfcounter_cycles cycles;
    timer_start(&cycles); // START TIMER
#endif
    // Single-thread reduction
    // Barrier
    mybarrier_wait();
    if (tasklet_id == 0) {
        T total = message[0];
        for (unsigned t = 1; t < NR_TASKLETS; t++) total += message[t];
        sk_log_write_idx(0, (uint64_t)total);
    	__ime_wait_for_host();
    }
    return 0;
}
