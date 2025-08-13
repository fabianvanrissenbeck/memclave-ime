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

#include "support/common.h"


// These must match the host definitions:
#define ARG_OFFSET     0x2000
#define ARG_SIZE       sizeof(dpu_arguments_t)
#define A_OFFSET       (ARG_OFFSET + ((ARG_SIZE + 0xFF) & ~0xFF))

#define TASKLETS 16

__host dpu_arguments_t DPU_INPUT_ARGUMENTS;

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

// GEMV
static void gemv(T *bufferC, T *bufferA, T *bufferB, int pos) {
	for (unsigned int i = 0; i < BLOCK_SIZE / sizeof(T); i++) {
		bufferC[pos] += bufferA[i] * bufferB[i];
	}
	return;
}

// main
int main() {
	unsigned int tasklet_id = me();
#if PRINT
	printf("tasklet_id = %u\n", tasklet_id);
#endif
	if (tasklet_id == 0){ // Initialize once the cycle counter
		mybarrier_init();
		mem_reset(); // Reset the heap
	}
	// read args from MRAM
        mram_read((__mram_ptr void const*)ARG_OFFSET,
                  &DPU_INPUT_ARGUMENTS,
                  sizeof(dpu_arguments_t));
	for(int i = 0; i<1000; i++);
	// Barrier
        mybarrier_wait();

	int32_t n_size = DPU_INPUT_ARGUMENTS.n_size;
	int32_t n_size_pad = DPU_INPUT_ARGUMENTS.n_size_pad;
	uint32_t nr_rows = DPU_INPUT_ARGUMENTS.nr_rows;
	uint32_t max_rows = DPU_INPUT_ARGUMENTS.max_rows;


	unsigned int nrows = nr_rows;
	unsigned int rows_per_tasklet; 
	unsigned int start_row;
	unsigned int chunks = nrows / (TASKLETS + TASKLETS);
	unsigned int dbl_chunks = chunks + chunks;                                                                       
	rows_per_tasklet = dbl_chunks;
	unsigned int rest_rows = nrows % (TASKLETS + TASKLETS);

	if ((tasklet_id + tasklet_id) < rest_rows)
		rows_per_tasklet += 2;
	if (rest_rows > 0) {
		if ((tasklet_id + tasklet_id) >= rest_rows) {
			unsigned int hlf_rest_rows = rest_rows >> 1;
			if ((rest_rows & 1) == 1)
				start_row = (hlf_rest_rows + 1) * (dbl_chunks + 2) + (tasklet_id - 1 - hlf_rest_rows) * dbl_chunks;
			else
				start_row = (hlf_rest_rows) * (dbl_chunks + 2) + (tasklet_id - hlf_rest_rows) * dbl_chunks;
		} else 
			start_row = tasklet_id * (dbl_chunks + 2);
	} else {
		start_row = tasklet_id * (dbl_chunks);
	}

        // Compute MRAM offsets (as set by host)
        uint32_t slice_bytes = max_rows * n_size_pad * sizeof(T);
        uint32_t vec_bytes   =     n_size_pad * sizeof(T);
 
        uint32_t base_A = A_OFFSET;
        uint32_t base_B = A_OFFSET + slice_bytes;
        uint32_t base_C = base_B + vec_bytes;

        // Address of the current row in MRAM
        uint32_t mram_base_addr_A = base_A + start_row * n_size_pad * sizeof(T);
        uint32_t mram_base_addr_B = base_B;
        uint32_t mram_base_addr_C = base_C + start_row * sizeof(T);
        uint32_t mram_temp_addr_A = mram_base_addr_A;
        uint32_t mram_temp_addr_B = mram_base_addr_B;

	// Inititalize a local cache to store the MRAM block
	T *cache_A = (T *) mem_alloc(BLOCK_SIZE + 8);
	T *cache_A_aux = (T *) mem_alloc(8);
	T *cache_B = (T *) mem_alloc(BLOCK_SIZE);
	T *cache_C = (T *) mem_alloc(2 * sizeof(T));

	int offset = 0;

	// Iterate over nr_rows
	for (unsigned int i = start_row; i < start_row + rows_per_tasklet; i += 2) {
		cache_C[0] = 0;
		cache_C[1] = 0;
		for(unsigned int pos = 0; pos < 2 && i + pos < nr_rows; pos++){
		        mram_temp_addr_A = (uint32_t) (base_A + (i + pos) * n_size_pad * sizeof(T));
		        mram_temp_addr_B = mram_base_addr_B;
			int n = 0, j;
			for (n = 0; n < (int32_t) (n_size - (BLOCK_SIZE/sizeof(T))); n += (BLOCK_SIZE / sizeof(T)))
			{

				mram_read((__mram_ptr void const*) (mram_temp_addr_A), cache_A, BLOCK_SIZE);
				mram_read((__mram_ptr void const*) (mram_temp_addr_B), cache_B, BLOCK_SIZE);

				if(offset)
				{

					for(unsigned int off = 0; off < (BLOCK_SIZE / sizeof(T)) - 1; off++)
					{
						cache_A[off] = cache_A[off + 1];
					}

					mram_read((__mram_ptr void const*) (mram_temp_addr_A + BLOCK_SIZE), cache_A_aux, 8);

					cache_A[BLOCK_SIZE / sizeof(T) - 1] = cache_A_aux[0];
				}

				// Compute GEMV
				gemv(cache_C, cache_A, cache_B, pos);

				// Update memory addresses
				mram_temp_addr_A += BLOCK_SIZE;
				mram_temp_addr_B += BLOCK_SIZE;
			}

			int remaining = n_size - n;
			mram_read((__mram_ptr void const*) (mram_temp_addr_A), cache_A, BLOCK_SIZE);


			if(offset)
			{
				for(unsigned int off = 0; off < (BLOCK_SIZE / sizeof(T)) -1; off++)
				{

					cache_A[off] = cache_A[off + 1];
				}

				mram_read((__mram_ptr void const*) (mram_temp_addr_A + BLOCK_SIZE ), cache_A_aux, 8);

  			       cache_A[BLOCK_SIZE / sizeof(T) - 1] = cache_A_aux[0];
			}


			mram_read((__mram_ptr void const*) (mram_temp_addr_B), cache_B, BLOCK_SIZE);

			for (j = 0; j < (int) (n_size - n); j++) {
				// Compute GEMV
				if(j >= (int)(BLOCK_SIZE / sizeof(T))){ 
					printf("error\n");
					break;
				}
				cache_C[pos] += cache_A[j] * cache_B[j];
			}


			mram_temp_addr_A += (BLOCK_SIZE - ((BLOCK_SIZE / sizeof(T)) - (n_size - n)) * sizeof(T));
			mram_temp_addr_B = mram_base_addr_B;

			if(mram_temp_addr_A % 8 != 0)
			{
				//offset = 1;
			}
			else
			{
				offset = 0;
			}
		}
		// Write cache to current MRAM block
		// Update memory address
		uint32_t rows_this_iter = 0;
		if (i < nr_rows) rows_this_iter++;
		if (i + 1 < nr_rows) rows_this_iter++;
		mram_write(cache_C, (__mram_ptr void *)(mram_base_addr_C), rows_this_iter * sizeof(T));
		mram_base_addr_C += rows_this_iter * sizeof(T);

	}

	return 0;
}
