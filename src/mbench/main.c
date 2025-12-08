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
#include <assert.h>

#define SUB 1
#define SK_LOG_ENABLED 1
#define TASKLETS 16
#include "support/common.h"
#include "support/cyclecount.h"
#include "support/log.h"

#define ARG_OFFSET   0x1000       // safe, at MRAM base
#define DATA_OFFSET  0x1100      // leave 256 B for your args
#define OUTPUT_OFFSET  (0x12000)      // leave 256 B for your args
#define RESULT_OFFSET  (0x23000)      // leave 256 B for your args
__host dpu_arguments_t DPU_INPUT_ARGUMENTS;
__host dpu_results_t DPU_RESULTS[TASKLETS];


// Shared state in WRAM
__attribute__((aligned(8)))
static volatile struct {
    volatile uint32_t arrive[TASKLETS];
    volatile uint32_t release_epoch;
} gbar;

// Call once at start (before first use)
static inline void mybarrier_init(void) {
    if (me() == 0) {
        for (uint32_t i = 0; i < TASKLETS; i++) gbar.arrive[i] = 0;
        gbar.release_epoch = 0;
    }
}

static inline void mybarrier_wait(void) {
    const uint32_t tid = me();
    // Every barrier use increments the epoch by 1
    const uint32_t want = gbar.release_epoch + 1;

    // Signal arrival (write *own* slot only)
    gbar.arrive[tid] = want;

    if (tid == 0) {
        // Wait for all tasklets to reach this epoch
        for (uint32_t i = 0; i < TASKLETS; i++) {
            while (gbar.arrive[i] != want) { /* spin */ }
        }
        gbar.release_epoch = want;
    } else {
        while (gbar.release_epoch != want) { /* spin */ }
    }
}

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

// main_kernel1
int main() {
    unsigned int tasklet_id = me();
    mybarrier_init();

    sk_log_init();
    sk_log_write_idx(0, 0);        // log thread/tasklet ID
    if (tasklet_id == 0){ // Initialize once the cycle counter
        //mem_reset(); // Reset the heap

        perfcounter_config(COUNT_CYCLES, true);
    }
    // dummy loop for starting all threads
    for (int i =0; i<100; i++);
    mybarrier_wait();
    perfcounter_cycles cycles;

    volatile uint64_t arg_size;
    volatile uint64_t __mram_ptr* args_ptr = (volatile uint64_t __mram_ptr*) (ARG_OFFSET);

    mram_read((__mram_ptr void const*)(args_ptr), &arg_size, sizeof(uint64_t));
    uint32_t input_size_dpu = arg_size / sizeof(T);
    T scalar = (T)input_size_dpu; // Simply use this number as a scalar

    dpu_results_t *result = &DPU_RESULTS[tasklet_id];
    result->cycles = 0;

    uint32_t mram_base_addr_A = (uint32_t)(DATA_OFFSET + (tasklet_id << BLOCK_SIZE_LOG2));
    uint32_t mram_base_addr_B = (uint32_t)(OUTPUT_OFFSET + (tasklet_id << BLOCK_SIZE_LOG2));
    uint32_t mram_base_addr_C = (uint32_t)(RESULT_OFFSET + (tasklet_id << BLOCK_SIZE_LOG2));

    // Initialize a local cache to store the MRAM block
    __attribute__((aligned(8))) static uint8_t _cache_pool_A[TASKLETS][BLOCK_SIZE];
    T *cache_A = (T *)_cache_pool_A[tasklet_id];
    volatile uint64_t __mram_ptr* test_ptr = (volatile uint64_t __mram_ptr*) ((64 << 20) - 64);
    test_ptr[0] = NR_TASKLETS;
    test_ptr[1] = input_size_dpu;
    test_ptr[2] = sizeof(T);

    for(unsigned int byte_index = 0; byte_index < input_size_dpu * sizeof(T); byte_index += BLOCK_SIZE * TASKLETS){

        // Load cache with current MRAM block
        mram_read((__mram_ptr void const*)(mram_base_addr_A + byte_index), cache_A, BLOCK_SIZE);

        // Barrier
        mybarrier_wait();
        timer_start(&cycles); // START TIMER

        // Update
        update(cache_A, scalar);

        result->cycles += timer_stop(&cycles); // STOP TIMER
        // Barrier
        mybarrier_wait();

        // Write cache to current MRAM block
        mram_write(cache_A, (__mram_ptr void*)(mram_base_addr_B + byte_index), BLOCK_SIZE);
    }
    //mram_write(result, (__mram_ptr void*)(mram_base_addr_C), sizeof(dpu_results_t));
    test_ptr[3] = result->cycles;
    //sk_log_write_idx(0, 24);        // log thread/tasklet ID

    return 0;
}
