/*
* Histogram (HST-S) with multiple tasklets
*
*/
#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <mram.h>
#include <alloc.h>
#include <perfcounter.h>
#include <barrier.h>

#define NR_TASKLETS  16
#include "support/common.h"
#include "support/log.h"
#include "support/mc_sync.h"


#define ARG_OFFSET   0x2000
#define ARG_SIZE     sizeof(dpu_arguments_t)
// align args to 0x100 boundary for safety
#define A_OFFSET     (ARG_OFFSET + ((ARG_SIZE + 0xFF) & ~0xFF))

// Array for communication between adjacent tasklets
uint32_t* message[NR_TASKLETS];
// DPU histogram
uint32_t* histo_dpu;

// Histogram in each tasklet
static void histogram(uint32_t* histo, uint32_t bins, T *input, unsigned int l_size){
    for(unsigned int j = 0; j < l_size; j++) {
        T d = input[j];
        histo[(d * bins) >> DEPTH] += 1;
    }
}

// main
int main() {
    unsigned int tasklet_id = me();
#if PRINT
    printf("tasklet_id = %u\n", tasklet_id);
#endif
    if (tasklet_id == 0){ // Initialize once the cycle counter
        mybarrier_init();
        mem_reset();
        sk_log_init();
    }
    // Barrier
    mybarrier_wait();

    dpu_arguments_t args;
    mram_read((__mram_ptr void const*)ARG_OFFSET, &args, sizeof(args));

    uint32_t input_size_dpu_bytes          = args.size;
    uint32_t input_size_dpu_bytes_transfer = args.transfer_size;
    uint32_t bins                          = args.bins;

    // Address of the current processing block in MRAM
    uint32_t base_tasklet = tasklet_id << BLOCK_SIZE_LOG2;
    uint32_t mram_base_addr_A = (uint32_t) A_OFFSET;
    uint32_t mram_base_addr_histo = (uint32_t) (A_OFFSET + input_size_dpu_bytes_transfer);

    // Initialize a local cache to store the MRAM block
    T *cache_A = (T *) mem_alloc(BLOCK_SIZE);
	
    // Local histogram
    uint32_t *histo = (uint32_t *) mem_alloc(bins * sizeof(uint32_t));

    // Initialize local histogram
    for(unsigned int i = 0; i < bins; i++){
        histo[i] = 0;
    }

    // Compute histogram
    for(unsigned int byte_index = base_tasklet; byte_index < input_size_dpu_bytes; byte_index += BLOCK_SIZE * NR_TASKLETS){

        // Bound checking
        uint32_t l_size_bytes = (byte_index + BLOCK_SIZE >= input_size_dpu_bytes) ? (input_size_dpu_bytes - byte_index) : BLOCK_SIZE;

        // Load cache with current MRAM block
        mram_read((const __mram_ptr void*)(mram_base_addr_A + byte_index), cache_A, l_size_bytes);

        // Histogram in each tasklet
        histogram(histo, bins, cache_A, l_size_bytes >> DIV);

    }
    message[tasklet_id] = histo;

    // Barrier
    mybarrier_wait();

    uint32_t *histo_dpu = message[0];

    for (unsigned int i = tasklet_id; i < bins; i += NR_TASKLETS){
        uint32_t b = 0;		
        for (unsigned int j = 0; j < NR_TASKLETS; j++){			
            b += *(message[j] + i);
        }
        histo_dpu[i] = b;
    }

    // Barrier
    mybarrier_wait();

    // Write dpu histogram to current MRAM block
    if(tasklet_id == 0){
        uint64_t count_sum = 0;
        for (unsigned i = 0; i < bins; ++i) count_sum += histo_dpu[i];
	
        sk_log_write_idx(4, count_sum);
        sk_log_write_idx(1, (uint64_t)histo_dpu[0]);
        sk_log_write_idx(2, (uint64_t)histo_dpu[1]);
        sk_log_write_idx(3, (uint64_t)histo_dpu[2]);
        if(bins * sizeof(uint32_t) <= 2048)
            mram_write(histo_dpu, (__mram_ptr void*)(mram_base_addr_histo), bins * sizeof(uint32_t));
        else 
            for(unsigned int offset = 0; offset < ((bins * sizeof(uint32_t)) >> 11); offset++){
                mram_write(histo_dpu + (offset << 9), (__mram_ptr void*)(mram_base_addr_histo + (offset << 11)), 2048);
            }
        sk_log_write_idx(0, 0xaaaa);
        __ime_wait_for_host();
    }

    return 0;
}
