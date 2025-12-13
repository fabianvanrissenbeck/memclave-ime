/*
* Scan with multiple tasklets (Reduce-scan-scan)
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
#include "support/mc_sync.h" 

#define ARG_OFFSET  0x2000
#define ARG_SIZE    ((uint32_t)sizeof(dpu_arguments_t))
#define A_OFFSET    (ARG_OFFSET + ((ARG_SIZE + 0xFF) & ~0xFF))

//enum { kernel0 = 0, kernel1 = 1 };
#define nr_kernels 2

static inline __attribute__((always_inline))
void read_args_aligned(dpu_arguments_t *args) {
    _Alignas(8) uint8_t buf[(sizeof(dpu_arguments_t) + 7u) & ~7u];
    mram_read((__mram_ptr void const*)ARG_OFFSET, buf, sizeof(buf));
    __builtin_memcpy(args, buf, sizeof(dpu_arguments_t));
}

// Array for communication between adjacent tasklets
T message[NR_TASKLETS];
T message_partial_count;

// Reduction in each tasklet
static T reduction(T *input){
    T output = 0;
    #pragma unroll
    for(unsigned int j = 0; j < REGS; j++) {
        output += input[j];
    }
    return output;
}
// Scan in each tasklet
static T scan(T *output, T *input){
    output[0] = input[0];
    #pragma unroll
    for(unsigned int j = 1; j < REGS; j++) {
        output[j] = output[j - 1] + input[j];
    }
    return output[REGS - 1];
}

// Add in each tasklet
static void add(T *output, T p_count){
    #pragma unroll
    for(unsigned int j = 0; j < REGS; j++) {
        output[j] += p_count;
    }
}

extern int main_kernel1(void);
extern int main_kernel2(void);

int (*kernels[nr_kernels])(void) = {main_kernel1, main_kernel2};

int main(void) {
    dpu_arguments_t args;
    read_args_aligned(&args);

    if (me() == 0) {
        mybarrier_init();
        handshake_init();
        mem_reset();
        sk_log_init();
    }
    mybarrier_wait();

    return kernels[args.kernel]();
}

// Reduction
int main_kernel1() {
    unsigned int tasklet_id = me();
#if PRINT
    printf("tasklet_id = %u\n", tasklet_id);
#endif
    // Barrier
    mybarrier_wait();

    dpu_arguments_t args;
    read_args_aligned(&args);
    const uint32_t input_size_dpu_bytes = args.size;

    const uint32_t base_tasklet = tasklet_id << BLOCK_SIZE_LOG2;
    const uint32_t mram_base_addr_A = (uint32_t)A_OFFSET;

    // Initialize a local cache to store the MRAM block
    T *cache_A = (T *) mem_alloc(BLOCK_SIZE);
	
    // Local count
    T l_count = 0;

    for(unsigned int byte_index = base_tasklet; byte_index < input_size_dpu_bytes; byte_index += BLOCK_SIZE * NR_TASKLETS){

        // Load cache with current MRAM block
        mram_read((const __mram_ptr void*)(mram_base_addr_A + byte_index), cache_A, BLOCK_SIZE);

        // Reduction in each tasklet
        l_count += reduction(cache_A);

    }

    // Reduce local counts
    message[tasklet_id] = l_count;

    // Single-thread reduction
    // Barrier
    mybarrier_wait();
    if(tasklet_id == 0){
        T total = message[0];
        for (unsigned t = 1; t < NR_TASKLETS; t++) total += message[t];
        /* publish per-DPU total to host via sk log index 0 */
        sk_log_write_idx(0, (uint64_t)total);
    }
    mybarrier_wait();

    return 0;
}

// Scan
int main_kernel2() {
    unsigned int tasklet_id = me();
#if PRINT
    printf("tasklet_id = %u\n", tasklet_id);
#endif
    // Barrier
    mybarrier_wait();

    dpu_arguments_t args;
    read_args_aligned(&args);

    const uint32_t input_size_dpu_bytes = args.size;
    const uint32_t A_base = (uint32_t)A_OFFSET;
    const uint32_t B_base = (uint32_t)(A_OFFSET + input_size_dpu_bytes);

    if (tasklet_id == (NR_TASKLETS - 1)) 
	    message_partial_count = args.t_count;
    mybarrier_wait();

    // Initialize a local cache to store the MRAM block
    T *cache_A = (T *) mem_alloc(BLOCK_SIZE);
    T *cache_B = (T *) mem_alloc(BLOCK_SIZE);
	
    const uint32_t base_tasklet = tasklet_id << BLOCK_SIZE_LOG2;

    for(unsigned int byte_index = base_tasklet; byte_index < input_size_dpu_bytes; byte_index += BLOCK_SIZE * NR_TASKLETS){
	if (tasklet_id == 0) { 
		g_epoch++; 
		__asm__ __volatile__("" ::: "memory"); 
	}
        mybarrier_wait();

        // Load cache with current MRAM block
        mram_read((const __mram_ptr void*)(A_base + byte_index), cache_A, BLOCK_SIZE);

        // Scan in each tasklet
        T l_count = scan(cache_B, cache_A); 

	T next_block_accum = 0;
        // Sync with adjacent tasklets
        T p_count = handshake_sync(l_count, tasklet_id, &next_block_accum);

        // Barrier
        mybarrier_wait();

        // Add in each tasklet
        add(cache_B, message_partial_count + p_count);

        // Write cache to current MRAM block
        mram_write(cache_B, (__mram_ptr void*)(B_base + byte_index), BLOCK_SIZE);

        // Total count in this DPU
        if(tasklet_id == NR_TASKLETS - 1){
            message_partial_count = message_partial_count + next_block_accum;
        }
	mybarrier_wait();
    }
		
    return 0;
}
