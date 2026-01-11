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
#include "barrier.h"

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

#define CTRL_OFFSET (ARG_OFFSET + 0x40u)

typedef struct __attribute__((aligned(8))) {
    uint32_t cmd;      // 0=IDLE, 1=RUN, 2=EXIT
    uint32_t job_id;
    uint32_t status;   // 0=WAITING, 1=RUNNING, 2=DONE, 3=EXITED
    uint32_t _pad;
} ctrl_t;

#define CMD_IDLE 0
#define CMD_RUN  1
#define CMD_EXIT 2
#define ST_WAITING 0
#define ST_RUNNING 1
#define ST_DONE    2
#define ST_EXITED  3

__dma_aligned volatile uint32_t g_phase = 0;
__dma_aligned ctrl_t g_ctrl;

static inline void wait_for_host_and_fetch_ctrl(void) {
    g_phase = 1;
    __ime_wait_for_host();     // yield + release mux, block until host wakes us
    g_phase = 0;
    mram_read((__mram_ptr void const*)CTRL_OFFSET, &g_ctrl, sizeof(g_ctrl));
}

// GEMV
static inline void gemv(T *bufferC, T *bufferA, T *bufferB, int pos) {
	//#pragma unroll
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

IME_BARRIER_INIT(bench_barrier, NR_TASKLETS)

// main
int main() {
    const uint32_t tasklet_id = me();

    if (tasklet_id == 0) {
        mem_reset();
        sk_log_init();
        // initial yield: host will fill args/A/B + set CMD_RUN
        wait_for_host_and_fetch_ctrl();
    }
    ime_barrier_wait(&bench_barrier);

    while (1) {
        if (g_ctrl.cmd == CMD_EXIT) break;
       
        if (tasklet_id == 0) {
            mem_reset();
            g_ctrl.status = ST_RUNNING;
            mram_write(&g_ctrl, (__mram_ptr void*)CTRL_OFFSET, sizeof(g_ctrl));
        }
        ime_barrier_wait(&bench_barrier);
       
        dpu_arguments_t args;
        read_args(&args);
       
        ime_barrier_wait(&bench_barrier);
       
        int32_t  n_size     = args.n_size;
        int32_t  n_size_pad = args.n_size_pad;
        uint32_t nr_rows    = args.nr_rows;
        uint32_t max_rows   = args.max_rows;
       
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
       
        uint32_t slice_bytes = max_rows * n_size_pad * sizeof(T);
        uint32_t base_A      = (uint32_t)A_OFFSET;
        uint32_t base_B      = base_A + slice_bytes;
        uint32_t base_C      = base_B + (uint32_t)(n_size_pad * sizeof(T));
       
        uint32_t mram_base_addr_C = base_C + start_row * sizeof(T);
       
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
       
            mram_write(cache_C, (__mram_ptr void *)mram_base_addr_C, 8);
            mram_base_addr_C += (uint32_t)(2 * sizeof(T));
        }
        ime_barrier_wait(&bench_barrier);
	if (tasklet_id == 0) {
            g_ctrl.status = ST_DONE;
            mram_write(&g_ctrl, (__mram_ptr void*)CTRL_OFFSET, sizeof(g_ctrl));
            wait_for_host_and_fetch_ctrl();     // yield until host sets up next layer or EXIT
        }
       ime_barrier_wait(&bench_barrier);
    }

    ime_barrier_wait(&bench_barrier);

    return 0;
}
