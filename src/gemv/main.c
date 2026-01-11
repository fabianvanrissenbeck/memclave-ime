#include <defs.h>
#include <stdint.h>
#include <assert.h>
#include <mram.h>
#include <alloc.h>
#include <perfcounter.h>
#include <barrier.h>

#define SK_LOG_ENABLED 1
#include "support/common.h"
#include "support/log.h"

#define NR_TASKLETS 16
#define roundup(n, m) ((n / m) * m + m)

/* MRAM layout */
#ifndef ARG_OFFSET
#define ARG_OFFSET 0x2000u
#endif
#ifndef ARG_SIZE
#define ARG_SIZE   (sizeof(dpu_arguments_t))
#endif
#ifndef A_OFFSET
#define A_OFFSET   (ARG_OFFSET + ((ARG_SIZE + 0xFFu) & ~0xFFu))
#endif

/* SK log (8 qwords) */
//#define LOG_WORDS 8
//#define LOG_MAGIC 0x534B4C4F475631ULL /* "SKLOGV1" */

typedef struct { volatile uint32_t v; uint32_t pad; } barrier_slot_t;

__attribute__((aligned(8)))
static struct {
    barrier_slot_t arrive[NR_TASKLETS];
    volatile uint32_t sense;
} gbar;

__dma_aligned static uint32_t tl_cycles[NR_TASKLETS];

static inline void mybarrier_init(void) {
    if (me() == 0) {
        gbar.sense = 0;
        for (uint32_t i = 0; i < NR_TASKLETS; i++) gbar.arrive[i].v = 1;
        __asm__ __volatile__("" ::: "memory");
    }
    while (gbar.sense != 0) { __asm__ __volatile__("" ::: "memory"); }
}

static inline void mybarrier_wait(void) {
    const uint32_t tid    = me();
    const uint32_t next_s = !gbar.sense;

    gbar.arrive[tid].v = next_s;
    __asm__ __volatile__("" ::: "memory");

    if (tid == 0) {
        for (uint32_t i = 0; i < NR_TASKLETS; i++) {
            while (__builtin_expect(gbar.arrive[i].v != next_s, 1))
                __asm__ __volatile__("" ::: "memory");
        }
        gbar.sense = next_s;
        __asm__ __volatile__("" ::: "memory");
    } else {
        while (__builtin_expect(gbar.sense != next_s, 1))
            __asm__ __volatile__("" ::: "memory");
    }
}

/* GEMV inner block */
static void gemv(T *bufferC, T *bufferA, T *bufferB, int pos) {
    for (unsigned int i = 0; i < BLOCK_SIZE / sizeof(T); i++) {
        bufferC[pos] += bufferA[i] * bufferB[i];
    }
}

/* safe aligned args read */
static inline void read_args(dpu_arguments_t *out) {
    _Alignas(8) uint8_t buf[(sizeof(dpu_arguments_t) + 7u) & ~7u];
    mram_read((__mram_ptr void const*)ARG_OFFSET, buf, sizeof(buf));
    __builtin_memcpy(out, buf, sizeof(dpu_arguments_t));
}

int main(void) {
    const uint32_t tasklet_id = me();

    if (tasklet_id == 0) {
        mybarrier_init();
        mem_reset();
        sk_log_init();
        perfcounter_config(COUNT_CYCLES, true);
    }

    mybarrier_wait();

    dpu_arguments_t args;
    read_args(&args);

    int32_t n_size     = args.n_size;
    int32_t n_size_pad = args.n_size_pad;
    uint32_t nr_rows   = args.nr_rows;
    uint32_t max_rows  = args.max_rows;

    /* bookkeeping */
    const unsigned int element_per_cacheC = 8 / sizeof(T);

    unsigned int nrows = nr_rows;
    unsigned int rows_per_tasklet;
    unsigned int start_row;

    unsigned int chunks      = nrows / (NR_TASKLETS * element_per_cacheC);
    unsigned int dbl_chunks  = chunks * element_per_cacheC;
    rows_per_tasklet         = dbl_chunks;
    unsigned int rest_rows   = nrows % (NR_TASKLETS * element_per_cacheC);

    if ((tasklet_id * element_per_cacheC) < rest_rows)
        rows_per_tasklet += element_per_cacheC;

    if (rest_rows > 0) {
        if ((tasklet_id * element_per_cacheC) >= rest_rows) {
            if ((rest_rows % element_per_cacheC) != 0)
                start_row = roundup(rest_rows, element_per_cacheC) + tasklet_id * dbl_chunks;
            else
                start_row = rest_rows + tasklet_id * dbl_chunks;
        } else {
            start_row = tasklet_id * (dbl_chunks + element_per_cacheC);
        }
    } else {
        start_row = tasklet_id * (dbl_chunks);
    }

    /* MRAM base addresses */
    const uint32_t slice_bytes = max_rows * n_size_pad * sizeof(T);
    const uint32_t vec_bytes   = n_size_pad * sizeof(T);

    const uint32_t mram_base_addr_A = (uint32_t)A_OFFSET;
    const uint32_t mram_base_addr_B = (uint32_t)(mram_base_addr_A + slice_bytes);
    uint32_t mram_base_addr_C       = (uint32_t)(mram_base_addr_B + vec_bytes + start_row * sizeof(T));

    uint32_t mram_temp_addr_A;
    uint32_t mram_temp_addr_B = mram_base_addr_B;

    /* Local caches */
    T *cache_A     = (T*)mem_alloc(BLOCK_SIZE + 8);
    T *cache_A_aux = (T*)mem_alloc(8);
    T *cache_B     = (T*)mem_alloc(BLOCK_SIZE);
    T *cache_C     = (T*)mem_alloc(8);

    int offset = 0;

    mybarrier_wait();
    uint32_t t0 = perfcounter_get();

    for (unsigned int i = start_row; i < start_row + rows_per_tasklet; i += element_per_cacheC) {

        mram_temp_addr_A = mram_base_addr_A + i * n_size * sizeof(T);
        mram_temp_addr_B = mram_base_addr_B;

        /* clear cache_C (up to 8 bytes) */
        for (unsigned int c = 0; c < element_per_cacheC; c++) {
            cache_C[c] = 0;
        }

        for (unsigned int pos = 0; pos < element_per_cacheC; pos++) {
            if (i + pos >= nr_rows) break;

            int n = 0, j;

            for (n = 0; n < (int32_t)(n_size - (BLOCK_SIZE / sizeof(T))); n += (BLOCK_SIZE / sizeof(T))) {

                mram_read((__mram_ptr void const*)(mram_temp_addr_A), cache_A, BLOCK_SIZE);
                mram_read((__mram_ptr void const*)(mram_temp_addr_B), cache_B, BLOCK_SIZE);

                if (offset) {
                    for (unsigned int off = 0; off < (BLOCK_SIZE / sizeof(T)) - 1; off++) {
                        cache_A[off] = cache_A[off + 1];
                    }
                    mram_read((__mram_ptr void const*)(mram_temp_addr_A + BLOCK_SIZE), cache_A_aux, 8);
                    cache_A[BLOCK_SIZE / sizeof(T) - 1] = cache_A_aux[0];
                }

                gemv(cache_C, cache_A, cache_B, (int)pos);

                mram_temp_addr_A += BLOCK_SIZE;
                mram_temp_addr_B += BLOCK_SIZE;
            }

            mram_read((__mram_ptr void const*)(mram_temp_addr_A), cache_A, BLOCK_SIZE);

            if (offset) {
                for (unsigned int off = 0; off < (BLOCK_SIZE / sizeof(T)) - 1; off++) {
                    cache_A[off] = cache_A[off + 1];
                }
                mram_read((__mram_ptr void const*)(mram_temp_addr_A + BLOCK_SIZE), cache_A_aux, 8);
                cache_A[BLOCK_SIZE / sizeof(T) - 1] = cache_A_aux[0];
            }

            mram_read((__mram_ptr void const*)(mram_temp_addr_B), cache_B, BLOCK_SIZE);

            for (j = 0; j < (int)(n_size - n); j++) {
                if (j >= (int)(BLOCK_SIZE / sizeof(T))) break;
                cache_C[pos] += cache_A[j] * cache_B[j];
            }

            mram_temp_addr_A += (BLOCK_SIZE - ((BLOCK_SIZE / sizeof(T)) - (n_size - n)) * sizeof(T));
            mram_temp_addr_B = mram_base_addr_B;

            offset = (mram_temp_addr_A % 8 != 0) ? 1 : 0;
        }

        /* writes exactly 8 bytes and advances by 8 */
        mram_write(cache_C, (__mram_ptr void*)(mram_base_addr_C), 8);
        mram_base_addr_C += 8;
    }

    mybarrier_wait();
    //uint32_t t1 = perfcounter_get();

    //tl_cycles[tasklet_id] = t1 - t0;
    //mybarrier_wait();

    if (tasklet_id == 0) {
        __ime_wait_for_host();
    }

    return 0;
}
