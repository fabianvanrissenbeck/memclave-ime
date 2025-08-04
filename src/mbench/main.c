/*
* Execution of arithmetic operations with multiple tasklets
*
*/
#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <mram.h>
#include <alloc.h>
#include <perfcounter.h>
#include <barrier.h>

#define SUB 1
#define SK_LOG_ENABLED 1
#include "support/common.h"
#include "support/cyclecount.h"
#include "support/log.h"

//#define NR_TASKLETS 1
#define ARG_OFFSET   0x1000       // safe, at MRAM base
#define DATA_OFFSET  0x1100      // leave 256 B for your args
#define OUTPUT_OFFSET  (0x12000)      // leave 256 B for your args
#define RESULT_OFFSET  (0x23000)      // leave 256 B for your args
__host dpu_arguments_t DPU_INPUT_ARGUMENTS;
__host dpu_results_t DPU_RESULTS[NR_TASKLETS];

// Arithmetic operation
static void update(T *bufferA, T scalar) {
    //#pragma unroll
    for (unsigned int i = 0; i < BLOCK_SIZE / sizeof(T); i++){
        // WRAM READ
        T temp = bufferA[i];
#ifdef ADD
        temp += scalar; // ADD 
#elif SUB
        temp -= scalar; // SUB
#elif MUL
        temp *= scalar; // MUL 
#elif DIV
        temp /= scalar; // DIV
#endif
        // WRAM WRITE
        bufferA[i] = temp;
    }
}

// Barrier
BARRIER_INIT(my_barrier, NR_TASKLETS);

extern int main_kernel1(void);

int (*kernels[nr_kernels])(void) = {main_kernel1};

int main(void) { 
    // Kernel
    //return kernels[DPU_INPUT_ARGUMENTS.kernel](); 
    return main_kernel1(); 
}

int main_old(void) {
    // assert(add(a, b) == 0x9ace1765afc17fa1 + 0x3b14d3aaf52bc131);

    volatile uint64_t __mram_ptr* test_ptr = (volatile uint64_t __mram_ptr*) ((64 << 20) - 64);
    test_ptr[0] = 72;
    test_ptr[1] = "E";
    test_ptr[2] = "Y";
    //assert(*test_ptr == 0xF0F0F0F0);

    asm("stop");
}

// main_kernel1
int main_kernel1() {
    unsigned int tasklet_id = me();

    sk_log_init();
    if (tasklet_id == 0){ // Initialize once the cycle counter
        mem_reset(); // Reset the heap

        perfcounter_config(COUNT_CYCLES, true);
    }
    // Barrier
    // FIXME
    //barrier_wait(&my_barrier);
    perfcounter_cycles cycles;

    volatile uint64_t arg_size;
    volatile uint64_t __mram_ptr* args_ptr = (volatile uint64_t __mram_ptr*) (ARG_OFFSET);

    //uint32_t input_size_dpu = DPU_INPUT_ARGUMENTS.size / sizeof(T);
    mram_read((__mram_ptr void const*)(args_ptr), &arg_size, sizeof(uint64_t));
    uint32_t input_size_dpu = arg_size / sizeof(T);
    T scalar = (T)input_size_dpu; // Simply use this number as a scalar

    dpu_results_t *result = &DPU_RESULTS[tasklet_id];
    result->cycles = 0;

    // Address of the current processing block in MRAM
    //uint32_t mram_base_addr_A = (uint32_t)(DPU_MRAM_HEAP_POINTER + (tasklet_id << BLOCK_SIZE_LOG2));
    //uint32_t mram_base_addr_B = (uint32_t)(DPU_MRAM_HEAP_POINTER + (tasklet_id << BLOCK_SIZE_LOG2) + input_size_dpu * sizeof(T));
    uint32_t mram_base_addr_A = (uint32_t)(DATA_OFFSET + (tasklet_id << BLOCK_SIZE_LOG2));
    uint32_t mram_base_addr_B = (uint32_t)(OUTPUT_OFFSET + (tasklet_id << BLOCK_SIZE_LOG2));
    uint32_t mram_base_addr_C = (uint32_t)(RESULT_OFFSET + (tasklet_id << BLOCK_SIZE_LOG2));

    // Initialize a local cache to store the MRAM block
    T *cache_A = (T *) mem_alloc(BLOCK_SIZE);
    volatile uint64_t __mram_ptr* test_ptr = (volatile uint64_t __mram_ptr*) ((64 << 20) - 64);
    test_ptr[0] = 73;
    test_ptr[1] = input_size_dpu;
    test_ptr[2] = sizeof(T);
    //volatile uint64_t __mram_ptr* args_ptr = (volatile uint64_t __mram_ptr*) (0x1000);
    //mram_read((__mram_ptr void const*)(args_ptr), cache_A, BLOCK_SIZE);
    //test_ptr[2] = *cache_A;

    for(unsigned int byte_index = 0; byte_index < input_size_dpu * sizeof(T); byte_index += BLOCK_SIZE * NR_TASKLETS){

        // Load cache with current MRAM block
        mram_read((__mram_ptr void const*)(mram_base_addr_A + byte_index), cache_A, BLOCK_SIZE);

        // Barrier
        //barrier_wait(&my_barrier);
        timer_start(&cycles); // START TIMER

        // Update
    	//test_ptr[1] = cache_A[0];
        update(cache_A, scalar);
    	//test_ptr[2] = scalar;
    	//test_ptr[3] = cache_A[0];

        result->cycles += timer_stop(&cycles); // STOP TIMER
        // Barrier
        //barrier_wait(&my_barrier);

        // Write cache to current MRAM block
        mram_write(cache_A, (__mram_ptr void*)(mram_base_addr_B + byte_index), BLOCK_SIZE);
    }
    mram_write(result, (__mram_ptr void*)(mram_base_addr_C), sizeof(dpu_results_t));
    test_ptr[3] = result->cycles;
    sk_log_write_idx(0, 23);        // log thread/tasklet ID

    return 0;
}
