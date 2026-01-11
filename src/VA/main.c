/*
* Vector addition with multiple tasklets
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
#include "support/common.h"
#include "support/log.h"
#include "support/mc_sync.h"


/* MRAM layout */
#define ARG_OFFSET  0x2000u
#define ARG_SIZE    ((uint32_t)sizeof(dpu_arguments_t))
#define A_OFFSET    (ARG_OFFSET + ((ARG_SIZE + 0xFFu) & ~0xFFu))

#define LOG_WORDS 8
#define LOG_MAGIC 0x534B4C4F475631ULL /* "SKLOGV1" */

__dma_aligned static uint32_t tl_cycles[NR_TASKLETS];

static inline __attribute__((always_inline))
void read_args_aligned(dpu_arguments_t *args) {
    _Alignas(8) uint8_t buf[(sizeof(dpu_arguments_t) + 7u) & ~7u];
    mram_read((__mram_ptr void const*)ARG_OFFSET, buf, sizeof(buf));
    __builtin_memcpy(args, buf, sizeof(dpu_arguments_t));
}

static inline __attribute__((always_inline))
void vector_addition(T *bufB, const T *bufA, unsigned int elems) {
    for (unsigned int i = 0; i < elems; i++) bufB[i] += bufA[i];
}

extern int main_kernel1(void);

int (*kernels[nr_kernels])(void) = {main_kernel1};

int main(void) { 
    // Kernel
    return main_kernel1();
}

// main_kernel1
int main_kernel1() {
    unsigned int tasklet_id = me();
#if PRINT
    printf("tasklet_id = %u\n", tasklet_id);
#endif
    if (tasklet_id == 0){
        mem_reset();
        sk_log_init();
        mybarrier_init();
    }
    mybarrier_wait();

    dpu_arguments_t args;
    read_args_aligned(&args);

    const uint32_t input_size_bytes         = (uint32_t)args.size;
    const uint32_t input_size_bytes_transfer = (uint32_t)args.transfer_size;

    /* A at A_OFFSET, B right after A-transfer region */
    const uint32_t base_tasklet = tasklet_id << BLOCK_SIZE_LOG2;
    const uint32_t mram_base_A  = (uint32_t)A_OFFSET;
    const uint32_t mram_base_B  = (uint32_t)(A_OFFSET + input_size_bytes_transfer);

    /* local caches */
    T *cache_A = (T*)mem_alloc(BLOCK_SIZE);
    T *cache_B = (T*)mem_alloc(BLOCK_SIZE);

    for (uint32_t byte_index = base_tasklet;
         byte_index < input_size_bytes;
         byte_index += BLOCK_SIZE * NR_TASKLETS) {

        const uint32_t l_size_bytes =
            (byte_index + BLOCK_SIZE >= input_size_bytes) ? (input_size_bytes - byte_index) : BLOCK_SIZE;

        mram_read((__mram_ptr void const*)(mram_base_A + byte_index), cache_A, l_size_bytes);
        mram_read((__mram_ptr void const*)(mram_base_B + byte_index), cache_B, l_size_bytes);

        vector_addition(cache_B, cache_A, (unsigned int)(l_size_bytes >> DIV));

        /* write back into B */
        mram_write(cache_B, (__mram_ptr void*)(mram_base_B + byte_index), l_size_bytes);
    }

    mybarrier_wait();

    if (tasklet_id == 0) {
        uint64_t mx = 0;
        for (int t = 0; t < NR_TASKLETS; t++)
            if (tl_cycles[t] > mx) mx = tl_cycles[t];
        __ime_wait_for_host();
    }
    return 0;
}
