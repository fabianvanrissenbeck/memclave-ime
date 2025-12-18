/*
 * Matrix vector multiplication with multiple tasklet
 *
 */
#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <mram.h>
#include <alloc.h>
#include <seqread.h>
#include <perfcounter.h>

#define SK_LOG_ENABLED 1
#define NR_TASKLETS 16
#include "support/common.h"
#include "support/log.h"
#include "support/mc_sync.h"


#define roundup(n, m) ((n / m) * m + m)

/* Must match host */
#ifndef ARG_OFFSET
#define ARG_OFFSET 0x2000u
#endif
#ifndef ARG_SIZE
#define ARG_SIZE   (sizeof(dpu_arguments_t))
#endif
#ifndef A_OFFSET
#define A_OFFSET   (ARG_OFFSET + ((ARG_SIZE + 0xFFu) & ~0xFFu))
#endif

// GEMV
static inline __attribute__((always_inline,optimize("O3,unroll-loops")))
void gemv(T *bufferC, T *bufferA, T *bufferB, int pos) {
	#pragma unroll
	for (unsigned int i = 0; i < BLOCK_SIZE / sizeof(T); i++) {
		bufferC[pos] += bufferA[i] * bufferB[i];
	}
	return;
}

static inline void read_args(dpu_arguments_t *out) {
    _Alignas(8) uint8_t buf[(sizeof(dpu_arguments_t) + 7u) & ~7u];
    mram_read((__mram_ptr void const*)ARG_OFFSET, buf, sizeof(buf));
    __builtin_memcpy(out, buf, sizeof(dpu_arguments_t));
}

// main
__attribute__((always_inline,optimize("O3,unroll-loops")))
int main() {
    const uint32_t tasklet_id = me();

    if (tasklet_id == 0) {
        mybarrier_init();
        mem_reset();
        sk_log_init();
        //perfcounter_config(COUNT_CYCLES, true);
    }
    mybarrier_wait();

    dpu_arguments_t args;
    read_args(&args);

    mybarrier_wait();
    //uint32_t s = perfcounter_get();

    int32_t  n_size     = args.n_size;
    int32_t  n_size_pad = args.n_size_pad;
    uint32_t nr_rows    = args.nr_rows;
    uint32_t max_rows   = args.max_rows;

    /* same partition math as PRiM MLP task.c */
    unsigned int nrows = nr_rows;
    unsigned int rows_per_tasklet;
    unsigned int start_row;

    unsigned int chunks     = nrows / (NR_TASKLETS + NR_TASKLETS);
    unsigned int dbl_chunks = chunks + chunks;
    rows_per_tasklet = dbl_chunks;

    unsigned int rest_rows = nrows % (NR_TASKLETS + NR_TASKLETS);

    if ((tasklet_id + tasklet_id) < rest_rows) rows_per_tasklet += 2;

    if (rest_rows > 0) {
        if ((tasklet_id + tasklet_id) >= rest_rows) {
            unsigned int hlf_rest_rows = rest_rows >> 1;
            if ((rest_rows & 1) == 1)
                start_row = (hlf_rest_rows + 1) * (dbl_chunks + 2) + (tasklet_id - 1 - hlf_rest_rows) * dbl_chunks;
            else
                start_row = (hlf_rest_rows) * (dbl_chunks + 2) + (tasklet_id - hlf_rest_rows) * dbl_chunks;
        } else {
            start_row = tasklet_id * (dbl_chunks + 2);
        }
    } else {
        start_row = tasklet_id * (dbl_chunks);
    }

    /* MRAM layout (PRiM-style, but base at A_OFFSET) */
    uint32_t slice_bytes = max_rows * n_size_pad * sizeof(T);
    uint32_t base_A      = (uint32_t)A_OFFSET;
    uint32_t base_B      = base_A + slice_bytes;
    uint32_t base_C      = base_B + (uint32_t)(n_size_pad * sizeof(T));

    uint32_t mram_base_addr_C = base_C + start_row * sizeof(T);

    /* caches (match PRiM) */
    T *cache_A     = (T *)mem_alloc(BLOCK_SIZE + 8);
    T *cache_A_aux = (T *)mem_alloc(8);
    T *cache_B     = (T *)mem_alloc(BLOCK_SIZE);
    T *cache_C     = (T *)mem_alloc(8);

    int offset = 0;

    for (unsigned int i = start_row; i < start_row + rows_per_tasklet; i += 2) {

        uint32_t mram_temp_addr_A = base_A + i * (uint32_t)(n_size * sizeof(T));
        uint32_t mram_temp_addr_B = base_B;

        cache_C[0] = 0;
        cache_C[1] = 0;

        for (unsigned int pos = 0; pos < 2 && (i + pos) < nr_rows; pos++) {
            int n = 0;

            for (n = 0; n < (int32_t)(n_size - (BLOCK_SIZE / sizeof(T))); n += (BLOCK_SIZE / sizeof(T))) {

                mram_read((__mram_ptr void const*)mram_temp_addr_A, cache_A, BLOCK_SIZE);
                mram_read((__mram_ptr void const*)mram_temp_addr_B, cache_B, BLOCK_SIZE);

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

            mram_read((__mram_ptr void const*)mram_temp_addr_A, cache_A, BLOCK_SIZE);

            if (offset) {
                for (unsigned int off = 0; off < (BLOCK_SIZE / sizeof(T)) - 1; off++) {
                    cache_A[off] = cache_A[off + 1];
                }
                mram_read((__mram_ptr void const*)(mram_temp_addr_A + BLOCK_SIZE), cache_A_aux, 8);
                cache_A[BLOCK_SIZE / sizeof(T) - 1] = cache_A_aux[0];
            }

            mram_read((__mram_ptr void const*)mram_temp_addr_B, cache_B, BLOCK_SIZE);

            for (int j = 0; j < (int)(n_size - n); j++) {
                if (j >= (int)(BLOCK_SIZE / sizeof(T))) break;
                cache_C[pos] += cache_A[j] * cache_B[j];
            }

            mram_temp_addr_A += (BLOCK_SIZE - ((BLOCK_SIZE / sizeof(T)) - (n_size - n)) * sizeof(T));
            mram_temp_addr_B = base_B;

            offset = (mram_temp_addr_A % 8 != 0) ? 1 : 0;
        }

        /* PRiM writes 8 bytes and advances by 2*sizeof(T) */
        mram_write(cache_C, (__mram_ptr void *)mram_base_addr_C, 8);
        mram_base_addr_C += (uint32_t)(2 * sizeof(T));
    }

    mybarrier_wait();
    //uint32_t e = perfcounter_get();

    //tl_cycles[tasklet_id] = e - s;
    mybarrier_wait();

    if (tasklet_id == 0) {
        uint64_t mx = 0;
        //for (int t = 0; t < NR_TASKLETS; t++) {
        //    if (tl_cycles[t] > mx) mx = tl_cycles[t];
        //}

        /* 64B SK log */
        //sk_log_write_idx(0, 0x534B4C4F475631ULL); /* "SKLOGV1" */
        //sk_log_write_idx(1, mx);
        //sk_log_write_idx(2, (uint64_t)s);
        //sk_log_write_idx(3, (uint64_t)e);
        //sk_log_write_idx(4, ((uint64_t)(uint32_t)n_size << 32) | (uint32_t)n_size_pad);
        //sk_log_write_idx(5, (uint64_t)nr_rows);
        //sk_log_write_idx(6, (uint64_t)NR_TASKLETS);
        //sk_log_write_idx(7, 1ULL);

        __ime_wait_for_host();
    }

    return 0;
}
