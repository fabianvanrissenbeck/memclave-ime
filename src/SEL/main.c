/*
* Select with multiple tasklets
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
#include "support/log.h"
#include "support/mc_sync.h"   // mybarrier_*, handshake_*, g_epoch

#define ARG_OFFSET  0x2000u
#define ARG_SIZE    ((uint32_t)sizeof(dpu_arguments_t))
#define A_OFFSET    (ARG_OFFSET + ((ARG_SIZE + 0xFFu) & ~0xFFu))

static inline __attribute__((always_inline))
void read_args_aligned(dpu_arguments_t *args) {
    _Alignas(8) uint8_t buf[(sizeof(dpu_arguments_t) + 7u) & ~7u];
    mram_read((__mram_ptr void const*)ARG_OFFSET, buf, sizeof(buf));
    __builtin_memcpy(args, buf, sizeof(dpu_arguments_t));
}

// Array for communication between adjacent tasklets
//uint32_t message[NR_TASKLETS];
uint32_t message_partial_count;

// SEL in each tasklet
static unsigned int select(T *output, T *input){
    unsigned int pos = 0;
    #pragma unroll
    for(unsigned int j = 0; j < REGS; j++) {
        if(!pred(input[j])) {
            output[pos] = input[j];
            pos++;
        }
    }
    return pos;
}

extern int main_kernel1(void);

int (*kernels[nr_kernels])(void) = {main_kernel1};

int main(void) { 
    dpu_arguments_t args;
    read_args_aligned(&args);

    if (me() == 0) {
        mybarrier_init();
        handshake_init();   // sets g_epoch and clears mailboxes
        mem_reset();
        sk_log_init();
    }
    mybarrier_wait();

    return kernels[0]();
}

// main_kernel1
int main_kernel1() {
    unsigned int tasklet_id = me();
#if PRINT
    printf("tasklet_id = %u\n", tasklet_id);
#endif
    // Barrier
    mybarrier_wait();

    dpu_arguments_t args;
    read_args_aligned(&args);
    const uint32_t input_size_bytes = args.size;

    // Address of the current processing block in MRAM
    uint32_t base_tasklet = tasklet_id << BLOCK_SIZE_LOG2;
    const uint32_t A_base = (uint32_t)A_OFFSET;
    const uint32_t B_base = (uint32_t)(A_OFFSET + input_size_bytes);

    // Initialize a local cache to store the MRAM block
    T *cache_A = (T *) mem_alloc(BLOCK_SIZE);
    T *cache_B = (T *) mem_alloc(BLOCK_SIZE);

    // Initialize shared variable
    if(tasklet_id == NR_TASKLETS - 1)
        message_partial_count = 0;
    // Barrier
    mybarrier_wait();

    for(unsigned int byte_index = base_tasklet; byte_index < input_size_bytes; byte_index += BLOCK_SIZE * NR_TASKLETS){
	if (tasklet_id == 0) { 
		g_epoch++; 
		__asm__ __volatile__("" ::: "memory"); 
	}
        mybarrier_wait();

        // Load cache with current MRAM block
        mram_read((__mram_ptr void const*)(A_base + byte_index), cache_A, BLOCK_SIZE);

        // SELECT in each tasklet
        uint32_t l_count = select(cache_B, cache_A); // In-place or out-of-place?

        // Sync with adjacent tasklets
        T next_block_accum = 0;
        const T p_count = handshake_sync((T)l_count, tasklet_id, &next_block_accum);

        // Barrier
        mybarrier_wait();

        // Write cache to current MRAM block
        const uint32_t elem_off = (uint32_t)(message_partial_count + p_count);
        const uint32_t byte_len = (uint32_t)(l_count * sizeof(T));
        mram_write(cache_B, (__mram_ptr void *)(B_base + (elem_off * sizeof(T))), byte_len);

        // Total count in this DPU
        if(tasklet_id == NR_TASKLETS - 1){
		message_partial_count = message_partial_count + next_block_accum;
        }
	mybarrier_wait();

    }

    mybarrier_wait();
    if (tasklet_id == 0) {
        sk_log_write_idx(0, (uint64_t)message_partial_count);
    }

    return 0;
}
