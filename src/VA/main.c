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

#include "support/common.h"
#include "support/log.h"

#ifndef ARG_OFFSET
#define ARG_OFFSET 0x1000
#define ARG_SIZE   (sizeof(dpu_arguments_t))
#define ARG_BYTES  ((ARG_SIZE + 7u) & ~7u)
#endif
#ifndef DATA_OFFSET
#define DATA_OFFSET 0x2000 // A
#endif
#ifndef OUTPUT_OFFSET
#define OUTPUT_OFFSET 0x200000 // B
#endif
#ifndef RESULT_OFFSET
#define RESULT_OFFSET 0x400000 // C
#endif

//#ifndef TASKLETS
#define TASKLETS 16
//#endif

//#ifndef NR_TASKLETS
#define NR_TASKLETS TASKLETS
//#endif

__host dpu_arguments_t DPU_INPUT_ARGUMENTS;
__host uint64_t tl_cycles[NR_TASKLETS];

typedef struct { volatile uint32_t v; uint32_t pad; } barrier_slot_t;
typedef union { dpu_arguments_t args; uint8_t pad[ARG_BYTES]; } _args_pad_u;
__dma_aligned _args_pad_u _argu;


__attribute__((aligned(8)))
static struct {
    barrier_slot_t arrive[TASKLETS];
    volatile uint32_t sense; // toggles 0/1 each barrier
} gbar;


static inline __attribute__((always_inline, optimize("O3")))
void mybarrier_init(void) {
    if (me() == 0) {
        gbar.sense = 0;
    for (uint32_t i = 0; i < TASKLETS; i++) gbar.arrive[i].v = 1; // opposite of sense
        __asm__ __volatile__("" ::: "memory");
    }
    while (gbar.sense != 0) { __asm__ __volatile__("" ::: "memory"); }
}


static inline __attribute__((always_inline, optimize("O3")))
void mybarrier_wait(void) {
    const uint32_t tid = me();
    const uint32_t next_s = !gbar.sense;
    gbar.arrive[tid].v = next_s; // single-writer slot
    __asm__ __volatile__("" ::: "memory");
    if (tid == 0) {
        for (uint32_t i = 0; i < TASKLETS; i++)
            while (__builtin_expect(gbar.arrive[i].v != next_s, 1))
                __asm__ __volatile__("" ::: "memory");
            gbar.sense = next_s; // release
            __asm__ __volatile__("" ::: "memory");
    } else {
        while (__builtin_expect(gbar.sense != next_s, 1))
            __asm__ __volatile__("" ::: "memory");
    }
}

// vector_addition: Computes the vector addition of a cached block 
static inline __attribute__((always_inline, optimize("O3,unroll-loops")))
void va_block(T *dst, const T *a, const T *b, unsigned elems) {
    #pragma unroll
    for (unsigned i = 0; i < elems; i++) dst[i] = a[i] + b[i];
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
        mybarrier_init();
    }
    mybarrier_wait();
    if (tasklet_id == 0){ // Initialize once the cycle counter
        mem_reset();
        sk_log_init();
        perfcounter_config(COUNT_CYCLES, true);
    }
    // Make sure everyone sees initialized state
    //for (int i = 0; i < 64; i++) __asm__ __volatile__("" ::: "memory");


    // Read args once from MRAM
    mram_read((__mram_ptr void const *)ARG_OFFSET, &_argu, ARG_BYTES);
    DPU_INPUT_ARGUMENTS = _argu.args;
#if 0
    if (tasklet_id == 0) {
        sk_log_write_idx(0, 0xA11A11A100000001ull);          // magic
        sk_log_write_idx(1, (uint64_t)DPU_INPUT_ARGUMENTS.size);
        sk_log_write_idx(2, (uint64_t)DPU_INPUT_ARGUMENTS.transfer_size);
        sk_log_write_idx(3, (uint64_t)DPU_INPUT_ARGUMENTS.kernel);
        sk_log_write_idx(7, 1ull);                           // done
    }
#endif


    const uint32_t size_bytes = (uint32_t)DPU_INPUT_ARGUMENTS.size; // bytes per DPU slice
    const uint32_t stride = BLOCK_SIZE * NR_TASKLETS; // bytes between this tasklet's blocks

    // Initialize a local cache to store the MRAM block
    __dma_aligned static T cacheA_[NR_TASKLETS][BLOCK_SIZE / sizeof(T)];
    __dma_aligned static T cacheB_[NR_TASKLETS][BLOCK_SIZE / sizeof(T)];
    __dma_aligned static T cacheC_[NR_TASKLETS][BLOCK_SIZE / sizeof(T)];
    T *cache_A = cacheA_[tasklet_id];
    T *cache_B = cacheB_[tasklet_id];
    T *cache_C = cacheC_[tasklet_id];

    const uint32_t start_off = tasklet_id * BLOCK_SIZE; // initial byte offset for this tasklet



    mybarrier_wait();
    uint32_t s = perfcounter_get();

    // Process this DPU's slice in interleaved BLOCK_SIZE chunks
    for (uint32_t off = start_off; off < size_bytes; off += stride) {
        // Compute chunk size (handle tail)
        const uint32_t remain = size_bytes - off;
        const uint32_t chunk_bytes = (remain >= BLOCK_SIZE) ? BLOCK_SIZE : remain;
	const uint32_t rd_bytes    = (chunk_bytes + 7u) & ~7u;


        // Base addresses for this chunk
        const uint32_t mram_base_addr_A = DATA_OFFSET + off;
        const uint32_t mram_base_addr_B = OUTPUT_OFFSET + off;
        const uint32_t mram_base_addr_C = RESULT_OFFSET + off;


        // Read A, B
        mram_read((__mram_ptr void const *)mram_base_addr_A, cache_A, rd_bytes);
        mram_read((__mram_ptr void const *)mram_base_addr_B, cache_B, rd_bytes);


	const unsigned elems = rd_bytes / sizeof(T);
        // Compute C = A + B
        va_block(cache_C, cache_A, cache_B, elems);


        // Write C (only valid region for the tail)
        mram_write(cache_C, (__mram_ptr void *)mram_base_addr_C, rd_bytes);
    }

    uint32_t e = perfcounter_get();
    mybarrier_wait();
    uint64_t dt = (uint64_t)(e - s);
    tl_cycles[tasklet_id] = dt;


    if (tasklet_id == 0) {
        // Reduce across tasklets
        uint64_t mx = 0;
        for (int t = 0; t < NR_TASKLETS; t++)
            if (tl_cycles[t] > mx) mx = tl_cycles[t];


            // 8×8B = 64B SK log layout (similar to MLP)
            // [0]=magic, [1]=whole_kernel_cycles_max, [2]=start_cycle, [3]=end_cycle,
            // [4]=(sizeof(T)<<32)|BLOCK_SIZE, [5]=bytes, [6]=tasklet_count, [7]=done=1
            sk_log_write_idx(0, 0xffffULL);
            sk_log_write_idx(1, mx);
            sk_log_write_idx(2, (uint64_t)s);
            sk_log_write_idx(3, (uint64_t)e);
            sk_log_write_idx(4, ((uint64_t)sizeof(T) << 32) | (uint32_t)BLOCK_SIZE);
            sk_log_write_idx(5, (uint64_t)size_bytes);
            sk_log_write_idx(6, (uint64_t)NR_TASKLETS);
            sk_log_write_idx(7, 1ULL);
    }
    return 0;
}
