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

#define ARG_OFFSET     0x2000         // leave MRAM[0x0000–0x0FFF] for anything else
#define ARG_SIZE       sizeof(dpu_arguments_t)  // e.g. 16

// Next free address, aligned up to 0x100 boundary
#define A_OFFSET       (ARG_OFFSET + ((ARG_SIZE + 0xFF) & ~0xFF))  // e.g. 0x1100

typedef struct { volatile uint32_t v; uint32_t pad; } barrier_slot_t;

__attribute__((aligned(8)))
static struct {
    barrier_slot_t arrive[NR_TASKLETS];
    volatile uint32_t sense;   // toggles 0/1 each barrier
} gbar;

// Call once at start (before first use)
static inline void mybarrier_init(void) {
    if (me() == 0) {
        gbar.sense = 0;
        // Initialize slots to the *opposite* of sense so first wait() completes correctly
        for (uint32_t i = 0; i < NR_TASKLETS; i++) gbar.arrive[i].v = 1;
        // Compiler fence so nobody hoists loads/stores around the init
        __asm__ __volatile__("" ::: "memory");
    }
    // Make sure all tasklets see initialized state (local spin; cheap)
    // Each non-zero tasklet waits until sense is 0 (already true),
    // but this also prevents later code from reordering across init.
    while (gbar.sense != 0) { __asm__ __volatile__("" ::: "memory"); }
}

static inline void mybarrier_wait(void) {
    const uint32_t tid      = me();
    const uint32_t next_s   = !gbar.sense;      // local snapshot of the target sense

    // Arrive: single-writer per slot, no atomics needed
    gbar.arrive[tid].v = next_s;
    __asm__ __volatile__("" ::: "memory");      // prevent store/load reordering

    if (tid == 0) {
        // Wait for all slots to match next_s (unrolled a bit to cut loop overhead)
        for (uint32_t i = 0; i < NR_TASKLETS; i++) {
            while (__builtin_expect(gbar.arrive[i].v != next_s, 1)) {
                __asm__ __volatile__("" ::: "memory");
            }
        }
        // Release: flip the global sense (single writer)
        gbar.sense = next_s;
        __asm__ __volatile__("" ::: "memory");
    } else {
        // Others: wait for the global sense flip
        while (__builtin_expect(gbar.sense != next_s, 1)) {
            __asm__ __volatile__("" ::: "memory");
        }
    }
}

// GEMV
static void gemv(T *bufferC, T *bufferA, T *bufferB, int pos) {
	for (unsigned int i = 0; i < BLOCK_SIZE / sizeof(T); i++) {
		bufferC[pos] += bufferA[i] * bufferB[i];
	}
	return;
}

// Barrier
//BARRIER_INIT(my_barrier, NR_TASKLETS);

// main
int main() {
	unsigned int tasklet_id = me();
	if (tasklet_id == 0){ // Initialize once the cycle counter
		mybarrier_init();
		mem_reset(); // Reset the heap
    		sk_log_init();
	}
#if 1
    volatile uint64_t __mram_ptr* test_ptr = (volatile uint64_t __mram_ptr*) ((64 << 20) - 64);
    test_ptr[0] = 72;
#endif
#if PRINT
	// printf("tasklet_id = %u\n", tasklet_id);
#endif
	//printf("HOST sizeof(T) = %zu-bytes\n", sizeof(T));
	if (tasklet_id == 0){ // Initialize once the cycle counter
		mem_reset(); // Reset the heap
	}
	dpu_arguments_t args;
	mram_read((__mram_ptr void const*)ARG_OFFSET, &args, sizeof(args));
        test_ptr[1] = args.n_size;

	// Barrier
	//barrier_wait(&my_barrier);

	int32_t n_size = args.n_size;
	int32_t n_size_pad = args.n_size_pad;
	uint32_t nr_rows = args.nr_rows;
	uint32_t max_rows = args.max_rows;
        //debug[0] = args.n_size;
        //debug[1] = args.n_size_pad;
        //debug[2] = args.nr_rows;
        //debug[3] = args.max_rows;


	unsigned int element_per_cacheC = 8/sizeof(T);

	unsigned int nrows = nr_rows;
	unsigned int rows_per_tasklet; 
	unsigned int start_row;
	unsigned int chunks = nrows / (NR_TASKLETS * element_per_cacheC);
	unsigned int dbl_chunks = chunks * element_per_cacheC; //chunks + chunks; 
	rows_per_tasklet = dbl_chunks;
	unsigned int rest_rows = nrows % (NR_TASKLETS * element_per_cacheC); //(NR_TASKLETS + NR_TASKLETS);

	if ((tasklet_id * element_per_cacheC) < rest_rows)
		rows_per_tasklet += element_per_cacheC;
	if (rest_rows > 0) {
		if ((tasklet_id * element_per_cacheC) >= rest_rows) {
			// unsigned int hlf_rest_rows = rest_rows >> 1;
			if ((rest_rows % element_per_cacheC) != 0)
				start_row = roundup(rest_rows, element_per_cacheC) + tasklet_id * dbl_chunks; 
			else
				start_row = rest_rows + tasklet_id * dbl_chunks; 
		} else 
			start_row = tasklet_id * (dbl_chunks + element_per_cacheC);
	} else {
		start_row = tasklet_id * (dbl_chunks);
	}

	// Address of the current row in MRAM
	uint32_t slice_bytes = args.max_rows   * args.n_size_pad * sizeof(T);
	uint32_t  vec_bytes   =           args.n_size_pad * sizeof(T);
	uint32_t mram_base_addr_A = (uint32_t) (A_OFFSET);
	uint32_t mram_base_addr_B = (uint32_t) (mram_base_addr_A + slice_bytes);
	uint32_t mram_base_addr_C =
    mram_base_addr_B + vec_bytes + start_row * sizeof(T);
	uint32_t mram_temp_addr_A = mram_base_addr_A;
	uint32_t mram_temp_addr_B = mram_base_addr_B;

	// Inititalize a local cache to store the MRAM block
	T *cache_A = (T *) mem_alloc(BLOCK_SIZE + 8);
	T *cache_A_aux = (T *) mem_alloc(8);
	T *cache_B = (T *) mem_alloc(BLOCK_SIZE);
	T *cache_C = (T *) mem_alloc(8);

	int offset = 0;

	#if PRINT
	printf("id: %d, rows_per_tasklet = %d\n",tasklet_id, rows_per_tasklet);
	printf("id: %d, start_row = %d\n",tasklet_id, start_row);
	#endif

	// Iterate over nr_rows
	for (unsigned int i = start_row; i < start_row + rows_per_tasklet; i += element_per_cacheC) {

		mram_temp_addr_A = mram_base_addr_A + i * n_size * sizeof(T);
		mram_temp_addr_B = mram_base_addr_B;

		// clear the cache
		for(unsigned int c = 0; c < element_per_cacheC; c++){
			cache_C[c] = 0; 
		}

		for(unsigned int pos = 0; pos < element_per_cacheC; pos++){ 
			if(i + pos >= nr_rows){
				// printf("id: %d, nrows: %d, error\n", tasklet_id, nrows);
				break;
			} 

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
				uint64_t sum = 0;
    				for (int i = 0; i < args.n_size; ++i) {
				        sum += (uint64_t)cache_A[i] * cache_B[i];
    				}

				// Update memory addresses
				mram_temp_addr_A += BLOCK_SIZE;
				mram_temp_addr_B += BLOCK_SIZE;
			}

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
					//printf("error\n");
					break;
				}
				cache_C[pos] += cache_A[j] * cache_B[j];
			}


			mram_temp_addr_A += (BLOCK_SIZE - ((BLOCK_SIZE / sizeof(T)) - (n_size - n)) * sizeof(T));
			mram_temp_addr_B = mram_base_addr_B;

			if(mram_temp_addr_A % 8 != 0)
			{
				offset = 1;
			}
			else
			{
				offset = 0;
			}
		}
		// Write cache to current MRAM block
		//mram_write(cache_C, (__mram_ptr void *) (mram_base_addr_C), 8);

		// Update memory address
		// mram_base_addr_C += 2 * sizeof(T);
		//mram_base_addr_C += 8; 
		mram_write(cache_C, (__mram_ptr void *) (mram_base_addr_C), element_per_cacheC * sizeof(T));
		mram_base_addr_C += element_per_cacheC * sizeof(T);

	}
	//debug[12] = cache_C[0];
        mybarrier_wait();
	if (tasklet_id == 0){ // Initialize once the cycle counter
        	//sk_log_write_idx(0, 0x5555);                    // "SKLOGV1"
        	sk_log_write_idx(0, cache_C[0]);                    // "SKLOGV1"
	}

	return 0;
}
