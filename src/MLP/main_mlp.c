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

#include "support/common.h"
#include "support/log.h"


// These must match the host definitions:
#define ARG_OFFSET     0x2000
#define ARG_SIZE       sizeof(dpu_arguments_t)
#define A_OFFSET       (ARG_OFFSET + ((ARG_SIZE + 0xFF) & ~0xFF))

#define TASKLETS 16

__host dpu_arguments_t DPU_INPUT_ARGUMENTS;
__host uint64_t tl_cycles[NR_TASKLETS];

typedef struct { volatile uint32_t v; uint32_t pad; } barrier_slot_t;

__attribute__((aligned(8)))
static struct {
    barrier_slot_t arrive[TASKLETS];
    volatile uint32_t sense;   // toggles 0/1 each barrier
} gbar;

// Call once at start (before first use)
static inline __attribute__((always_inline, optimize("O3")))
void mybarrier_init(void) {
    if (me() == 0) {
        gbar.sense = 0;
        // Initialize slots to the *opposite* of sense so first wait() completes correctly
        for (uint32_t i = 0; i < TASKLETS; i++) gbar.arrive[i].v = 1;
        // Compiler fence so nobody hoists loads/stores around the init
        __asm__ __volatile__("" ::: "memory");
    }
    // Make sure all tasklets see initialized state (local spin; cheap)
    // Each non-zero tasklet waits until sense is 0 (already true),
    // but this also prevents later code from reordering across init.
    while (gbar.sense != 0) { __asm__ __volatile__("" ::: "memory"); }
}

static inline __attribute__((always_inline, optimize("O3")))
void mybarrier_wait(void) {
    const uint32_t tid      = me();
    const uint32_t next_s   = !gbar.sense;      // local snapshot of the target sense

    // Arrive: single-writer per slot, no atomics needed
    gbar.arrive[tid].v = next_s;
    __asm__ __volatile__("" ::: "memory");      // prevent store/load reordering

    if (tid == 0) {
        // Wait for all slots to match next_s (unrolled a bit to cut loop overhead)
        for (uint32_t i = 0; i < TASKLETS; i++) {
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
static inline __attribute__((always_inline,optimize("O3,unroll-loops")))
void gemv(T *bufferC, T *bufferA, T *bufferB, int pos) {
	#pragma unroll
	for (unsigned int i = 0; i < BLOCK_SIZE / sizeof(T); i++) {
		bufferC[pos] += bufferA[i] * bufferB[i];
	}
	return;
}

// Count 'want' perf ticks using wrap-safe accumulation.
// Mark noinline and add a compiler barrier in the loop.
#if 0
__attribute__((optnone))
__attribute__((noinline))
uint64_t spin_for_ticks(uint64_t want) {
    // If want==0, do nothing but still read once
    volatile uint32_t last = perfcounter_get();
    volatile uint64_t acc  = 0;
    volatile uint64_t junk = 0;   // side-effect so loop body can't vanish

    while (want && acc < want) {
        uint32_t cur = perfcounter_get();
        acc += (uint32_t)(cur - last);  // unsigned -> wrap-safe delta
        last = cur;
        junk++;                         // consume a tiny bit of work
        // Compiler barrier: don't let the loop be “optimized”
        __asm__ __volatile__("" ::: "memory");
    }
    // touch junk so it isn't dead-stored
    if (junk == (uint64_t)-1) __asm__ __volatile__("" ::: "memory");
    return (uint64_t)acc;
}
#endif


volatile uint32_t t_spin1 = 0xbb;
volatile uint32_t t_spin0 = 0xaa;
// main
__attribute__((always_inline,optimize("O3,unroll-loops")))
int main() {
	unsigned int tasklet_id = me();
	unsigned int tasklet_id2 = me();
#if PRINT
	printf("tasklet_id = %u\n", tasklet_id);
#endif
	if (tasklet_id == 0){ // Initialize once the cycle counter
		mybarrier_init();
		mem_reset(); // Reset the heap
		sk_log_init();
		perfcounter_config(COUNT_CYCLES, true);
	}
	for(int i = 0; i<100; i++);
	// read args from MRAM
        mram_read((__mram_ptr void const*)ARG_OFFSET,
                  &DPU_INPUT_ARGUMENTS,
                  sizeof(dpu_arguments_t));
	// ---- Spin preamble (0-cost when spin_cycles == 0) ----
	mybarrier_wait();                    // make everyone see the args
#if 0
	uint64_t want = 0, acc = 0;
        if (me() == 0) {
            t_spin0 = perfcounter_get();
	    want    = DPU_INPUT_ARGUMENTS.spin_cycles;   // MRAM arg (already copied)
	    acc     = spin_for_ticks(want);
	    t_spin1 = perfcounter_get();
	}
	//mybarrier_wait();                    // ensure all start compute together
#endif
	// -------------------------------------------------------
	// Barrier
        //mybarrier_wait();
	uint32_t s = perfcounter_get();
#if 1

	int32_t n_size = DPU_INPUT_ARGUMENTS.n_size;
	int32_t n_size_pad = DPU_INPUT_ARGUMENTS.n_size_pad;
	uint32_t nr_rows = DPU_INPUT_ARGUMENTS.nr_rows;
	uint32_t max_rows = DPU_INPUT_ARGUMENTS.max_rows;


#if 1
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
#else
        const unsigned nrows = nr_rows;
        const unsigned start_row = (tasklet_id * nrows) / TASKLETS;
        const unsigned end_row   = ((tasklet_id + 1) * nrows) / TASKLETS;
#endif

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
        //uint32_t mram_temp_addr_A = mram_base_addr_A;
        //uint32_t mram_temp_addr_B = mram_base_addr_B;

	// Inititalize a local cache to store the MRAM block
	__dma_aligned T *cache_A = (T *) mem_alloc(BLOCK_SIZE);
	__dma_aligned T *cache_B = (T *) mem_alloc(BLOCK_SIZE);
	__dma_aligned T *cache_C = (T *) mem_alloc(2 * sizeof(T));
	cache_A = (T *)__builtin_assume_aligned(cache_A, 8);
	cache_B = (T *)__builtin_assume_aligned(cache_B, 8);
	cache_C = (T *)__builtin_assume_aligned(cache_C, 8);


	//int offset = 0;

#if 0
	const unsigned blk_elems = BLOCK_SIZE / sizeof(T);
	// Iterate over nr_rows
	#pragma unroll
	//for (unsigned int i = start_row; i < start_row + rows_per_tasklet; i += 2) {
	for (unsigned int i = start_row; i < end_row; i += 2) {
		cache_C[0] = 0;
		cache_C[1] = 0;
		#pragma unroll
		//for(unsigned int pos = 0; pos < 2 && i + pos < nr_rows; pos++){
		for(unsigned int pos = 0; pos < 2 && i + pos < end_row; pos++){
		        mram_temp_addr_A = (uint32_t) (base_A + (i + pos) * n_size_pad * sizeof(T));
		        mram_temp_addr_B = mram_base_addr_B;
			int n = 0, j;
			#pragma unroll
			for (n = 0; n < (int32_t) (n_size - blk_elems); n += blk_elems)
			{

				mram_read((__mram_ptr void const*) (mram_temp_addr_A), cache_A, BLOCK_SIZE);
				mram_read((__mram_ptr void const*) (mram_temp_addr_B), cache_B, BLOCK_SIZE);

				// Compute GEMV
				gemv(cache_C, cache_A, cache_B, pos);

				// Update memory addresses
				mram_temp_addr_A += BLOCK_SIZE;
				mram_temp_addr_B += BLOCK_SIZE;
			}

			int remaining = n_size - n;
			mram_read((__mram_ptr void const*) (mram_temp_addr_A), cache_A, BLOCK_SIZE);
			mram_read((__mram_ptr void const*) (mram_temp_addr_B), cache_B, BLOCK_SIZE);

			#pragma unroll
			for (j = 0; j < (int) (n_size - n); j++) {
				cache_C[pos] += cache_A[j] * cache_B[j];
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
#else
// ---- fast path: two-row accumulation; per-tasklet B streaming (no seqread, no barriers) ----
const unsigned blk_elems = BLOCK_SIZE / sizeof(T);

// Iterate assigned rows, 2 at a time so C writes stay contiguous
for (unsigned i = start_row; i < start_row + rows_per_tasklet; i += 2) {
    const uint32_t have_row0 = (i < nr_rows);
    const uint32_t have_row1 = ((i + 1) < nr_rows);
    if (!have_row0 && !have_row1) break;

    // Accumulators (one write to C at the end)
    T acc0 = 0, acc1 = 0;

    // Base MRAM addresses for these rows and for B
    uint32_t A0 = A_OFFSET + i * n_size_pad * sizeof(T);
    uint32_t A1 = A_OFFSET + (i + 1) * n_size_pad * sizeof(T);
    uint32_t Bp = A_OFFSET + (max_rows * n_size_pad * sizeof(T));  // base_B

    int n = 0;

    // Full blocks: read B once per block; then A0 and (optionally) A1 and accumulate
    for (n = 0; n < (int32_t)(n_size - blk_elems); n += (int32_t)blk_elems) {
        mram_read((__mram_ptr void const *)Bp, cache_B, BLOCK_SIZE);

        if (have_row0) {
            mram_read((__mram_ptr void const *)A0, cache_A, BLOCK_SIZE);
            #pragma unroll
            for (unsigned k = 0; k < blk_elems; k++) acc0 += cache_A[k] * cache_B[k];
            A0 += BLOCK_SIZE;
        }

        if (have_row1) {
            mram_read((__mram_ptr void const *)A1, cache_A, BLOCK_SIZE);
            #pragma unroll
            for (unsigned k = 0; k < blk_elems; k++) acc1 += cache_A[k] * cache_B[k];
            A1 += BLOCK_SIZE;
        }

        Bp += BLOCK_SIZE;
    }

    // Tail (<= blk_elems)
    const int rem = (int)n_size - n;
    if (rem > 0) {
        mram_read((__mram_ptr void const *)Bp, cache_B, BLOCK_SIZE);

        if (have_row0) {
            mram_read((__mram_ptr void const *)A0, cache_A, BLOCK_SIZE);
            #pragma unroll
            for (int j = 0; j < rem; j++) acc0 += cache_A[j] * cache_B[j];
        }
        if (have_row1) {
            mram_read((__mram_ptr void const *)A1, cache_A, BLOCK_SIZE);
            #pragma unroll
            for (int j = 0; j < rem; j++) acc1 += cache_A[j] * cache_B[j];
        }
    }

    // Contiguous writeback to C (1 or 2 rows)
    if (have_row0) cache_C[0] = acc0;
    if (have_row1) cache_C[1] = acc1;

    const uint32_t rows_this_iter = have_row0 + have_row1;
    mram_write(cache_C, (__mram_ptr void *)mram_base_addr_C, rows_this_iter * sizeof(T));
    mram_base_addr_C += rows_this_iter * sizeof(T);
}

#endif 
#endif
#if 1
        mybarrier_wait();
        uint32_t e = perfcounter_get();
        uint64_t my = e - s;
        tl_cycles[me()] = my;
    if (me() == 0) {
        uint64_t mx = UINT_MAX;
        for (int t = 0; t < NR_TASKLETS; t++)
            if (tl_cycles[t] < mx) mx = tl_cycles[t];

        // Layout: 8×8B = 64B
        // [0]=magic, [1]=whole_kernel_cycles_max, [2]=s, [3]=e, [4]=(tasklets<<32)|layers,
        // [5]=0 (spare), [6]=0 (spare), [7]=1 (done)
        sk_log_write_idx(0, 0xffffULL);                    // "SKLOGV1"
        sk_log_write_idx(1, mx);
        sk_log_write_idx(2, (uint64_t)s);
        //sk_log_write_idx(2, (uint64_t)DPU_INPUT_ARGUMENTS.dummy);
        //sk_log_write_idx(3, (uint64_t)e);
        sk_log_write_idx(3, (uint64_t)DPU_INPUT_ARGUMENTS.spin_cycles);
	sk_log_write_idx(4, (uint64_t)(t_spin1 - t_spin0));
	//sk_log_write_idx(5, (uint64_t)acc);             // compiled tasklet count
	//sk_log_write_idx(4, (uint64_t)t_spin0);
	//sk_log_write_idx(5, (uint64_t)(t_spin1));             // compiled tasklet count
        sk_log_write_idx(6, ((uint64_t)sizeof(T) << 32) | (uint32_t)BLOCK_SIZE);
        sk_log_write_idx(7, 1ULL);
    }
#endif

	return 0;
}
