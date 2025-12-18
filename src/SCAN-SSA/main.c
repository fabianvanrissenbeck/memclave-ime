/*
* Scan with multiple tasklets (Scan-scan-add)
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
#include "support/mc_sync.h"
#include "support/log.h"

#define ARG_OFFSET  0x2000
#define ARG_SIZE    ((uint32_t)sizeof(dpu_arguments_t))
#define A_OFFSET    (ARG_OFFSET + ((ARG_SIZE + 0xFF) & ~0xFF))

static inline __attribute__((always_inline))
void read_args_aligned(dpu_arguments_t *args) {
    _Alignas(8) uint8_t buf[(sizeof(dpu_arguments_t) + 7u) & ~7u];
    mram_read((__mram_ptr void const*)ARG_OFFSET, buf, sizeof(buf));
    __builtin_memcpy(args, buf, sizeof(dpu_arguments_t));
}

static T message_partial_count;

// Scan in each tasklet
static inline __attribute__((always_inline))
T scan_regs(T *output, const T *input) {
    output[0] = input[0];
#pragma unroll
    for (unsigned j = 1; j < REGS; j++) {
        output[j] = output[j - 1] + input[j];
    }
    return output[REGS - 1];
}

static inline __attribute__((always_inline))
void add_regs(T *io, T p_count) {
#pragma unroll
    for (unsigned j = 0; j < REGS; j++) io[j] += p_count;
}

extern int main_kernel1(void);
extern int main_kernel2(void);

int (*kernels[nr_kernels])(void) = {main_kernel1, main_kernel2};

dpu_arguments_t args;
int main(void) {
    read_args_aligned(&args);
    // init once
    if (me() == 0) {
        mybarrier_init();
        handshake_init();
        mem_reset();
        sk_log_init();
    }
    mybarrier_wait();

    return kernels[args.kernel]();
}

// Scan-(handshake)scan
int main_kernel1() {
#if 1 // Comment out for appendix experiment
    unsigned int tasklet_id = me();
#if PRINT
    printf("tasklet_id = %u\n", tasklet_id);
#endif
    //dpu_arguments_t args;
    //mram_read((__mram_ptr void const*)ARG_OFFSET, &args, sizeof(args));
    //read_args_aligned(&args);

    const uint32_t input_size_bytes = args.size;
    const uint32_t A_base = (uint32_t)A_OFFSET;
    const uint32_t B_base = (uint32_t)(A_OFFSET + input_size_bytes);

    if (tasklet_id == (NR_TASKLETS - 1)) message_partial_count = args.t_count;
    mybarrier_wait();

    // Initialize a local cache to store the MRAM block
    T *cache_A = (T *) mem_alloc(BLOCK_SIZE);
    T *cache_B = (T *) mem_alloc(BLOCK_SIZE);

    const uint32_t base_tasklet = tasklet_id << BLOCK_SIZE_LOG2;
	
    for (uint32_t byte_idx = base_tasklet; byte_idx < input_size_bytes; byte_idx += (BLOCK_SIZE * NR_TASKLETS)) {

	if (tasklet_id == 0) {
            g_epoch++;
            __asm__ __volatile__("" ::: "memory");
        }
        mybarrier_wait();
        // Load cache with current MRAM block
	mram_read((const __mram_ptr void*)(A_base + byte_idx), cache_A, BLOCK_SIZE);

        // Scan in each tasklet
        T l_count = scan_regs(cache_B, cache_A); 

        // Sync with adjacent tasklets
	T next_block_accum = 0;
        T p_count = handshake_sync(l_count, tasklet_id, &next_block_accum);

        // Barrier
	mybarrier_wait();

        // Add in each tasklet
        add_regs(cache_B, message_partial_count + p_count);

        // Write cache to current MRAM block
	mram_write(cache_B, (__mram_ptr void*)(B_base + byte_idx), BLOCK_SIZE);

        // Total count in this DPU
        if (tasklet_id == (NR_TASKLETS - 1)) {
            message_partial_count = message_partial_count + next_block_accum;
        }
        mybarrier_wait();
    }

    if (tasklet_id == 0) {
        sk_log_write_idx(0, (uint64_t)message_partial_count);
    	__ime_wait_for_host();
    }

#endif
    return 0;
}

// Add
int main_kernel2() {
    unsigned int tasklet_id = me();
#if PRINT
    printf("tasklet_id = %u\n", tasklet_id);
#endif
    dpu_arguments_t args;
    read_args_aligned(&args);

    const uint32_t input_size_bytes = args.size;
    const uint32_t B_base = (uint32_t)(A_OFFSET + input_size_bytes);

    T *cache_B = (T*)mem_alloc(BLOCK_SIZE);
    const uint32_t base_tasklet = tasklet_id << BLOCK_SIZE_LOG2;

    for (uint32_t byte_idx = base_tasklet; byte_idx < input_size_bytes; byte_idx += (BLOCK_SIZE * NR_TASKLETS)) {

        // Load cache with current MRAM block
	mram_read((const __mram_ptr void*)(B_base + byte_idx), cache_B, BLOCK_SIZE);

        // Add in each tasklet
        add_regs(cache_B, args.t_count);

        // Write cache to current MRAM block
	mram_write(cache_B, (__mram_ptr void*)(B_base + byte_idx), BLOCK_SIZE);

    }
    mybarrier_wait();
    if (tasklet_id == 0) {
        sk_log_write_idx(1, (uint64_t)args.t_count); // debug
    	__ime_wait_for_host();
    }

    return 0;
}
