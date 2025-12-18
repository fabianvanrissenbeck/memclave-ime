/*
* Binary Search with multiple tasklets
*
*/
#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <mram.h>
#include <alloc.h>
#include <mram.h>
#include <barrier.h>
#include <perfcounter.h>

/* debug slots: 0 is already used for final result; use 1..7 */
#define DBG_QINFO   1  /* [tasklet<<48 | q_low16<<32 | (uint32)searching_for_low32] */
#define DBG_BOUNDS  2  /* [uint32(R-L) << 32 | uint32(mid-L)] */
#define DBG_BRANCH  3  /* [state<<32 | (uint32)found_low32]   */
#define DBG_BASES   4  /* [uint32(A_base) << 32 | uint32(A_end)] */
#define DBG_ADDRS   5  /* [uint32(L) << 32 | uint32(R)]       */
#define DBG_TAIL    6  /* [uint32(remain) << 32 | uint32(start2 - A_base)] */
#define DBG_MARKER  7  /* arbitrary breadcrumbs */


#define NR_TASKLETS 16

#include "support/common.h"
#define T DTYPE
#include "support/mc_sync.h"
#include "support/log.h"

#define ARG_OFFSET  0x2000u
#define ARG_SIZE    ((uint32_t)sizeof(dpu_arguments_t))
#define A_OFFSET    (ARG_OFFSET + ((ARG_SIZE + 0xFFu) & ~0xFFu))
#define WORD_MASK   0xfffffff8u

static inline __attribute__((always_inline))
void read_args_aligned(dpu_arguments_t *args) {
    _Alignas(8) uint8_t buf[(sizeof(dpu_arguments_t) + 7u) & ~7u];
    mram_read((__mram_ptr void const*)ARG_OFFSET, buf, sizeof(buf));
    __builtin_memcpy(args, buf, sizeof(dpu_arguments_t));
}

volatile dpu_results_t DPU_RESULTS[NR_TASKLETS];

// Search
static DTYPE search(DTYPE *bufferA, DTYPE searching_for, size_t search_size) {
  DTYPE found = -2;
  if(bufferA[0] <= searching_for)
  {
    found = -1;
    for (uint32_t i = 0; i < search_size / sizeof(DTYPE); i++){
      if(bufferA[i] == searching_for)
      {
        found = i;
        break;
      }
    }
  }
  return found;
}

int main(void){
  unsigned int tasklet_id = me();
  if (tasklet_id == 0) {
        mem_reset();
        sk_log_init();
	mybarrier_init();
	for (unsigned t = 0; t < NR_TASKLETS; ++t)
            DPU_RESULTS[t].found = (DTYPE)-1;
  }
  #if PRINT
  printf("tasklet_id = %u\n", tasklet_id);
  #endif
  // Barrier
  mybarrier_wait();

  dpu_arguments_t args;
  read_args_aligned(&args);

  // Address of the current processing block in MRAM
  const uint64_t input_size = args.input_size;
  const uint32_t A_base     = (uint32_t)A_OFFSET;
  const uint32_t A_end      = A_base + (uint32_t)(input_size * sizeof(DTYPE));
  uint32_t query_addr       = A_end + tasklet_id * (uint32_t)((args.slice_per_dpu / NR_TASKLETS) * sizeof(DTYPE));
#if 0
    if (me() == 0){ 
	    sk_log_write_idx(0, 0xaaaa);
	    sk_log_write_idx(1, input_size);
	    sk_log_write_idx(2, A_base);
	    sk_log_write_idx(3, A_end);
	    sk_log_write_idx(4, query_addr);
	    sk_log_write_idx(5, args.slice_per_dpu);
    }
    return 0;
#endif

  // Initialize a local cache to store the MRAM block
  DTYPE *cache_A     = (DTYPE *) mem_alloc(BLOCK_SIZE);
  DTYPE *cache_aux_A = (DTYPE *) mem_alloc(BLOCK_SIZE);
  DTYPE *cache_aux_B = (DTYPE *) mem_alloc(BLOCK_SIZE);

  dpu_results_t *result = &DPU_RESULTS[tasklet_id];

  const uint64_t queries_per_tasklet = args.slice_per_dpu / NR_TASKLETS;
  for (uint64_t t = 0; t < queries_per_tasklet; t++) {
    _Alignas(8) DTYPE searching_for;
    mram_read((__mram_ptr void const *)query_addr, &searching_for, sizeof(DTYPE));
    query_addr += sizeof(DTYPE);

    /* reset bounds for each query */
    uint32_t L = A_base;
    uint32_t R = A_end;

    /* prefetch first/last blocks */
    mram_read((__mram_ptr void const *)L, cache_aux_A, BLOCK_SIZE);
    mram_read((__mram_ptr void const *)(R - BLOCK_SIZE), cache_aux_B, BLOCK_SIZE);
    //if (me() == 0)sk_log_write_idx(0, L);
    //if (me() == 0)sk_log_write_idx(1, R);
    while(1) 
    { 
	    // Locate the address of the mid mram block 
	    uint32_t mid = (L + R) / 2; 
	    mid &= WORD_MASK; 
	    // Boundary check 
	    if (mid < (L + BLOCK_SIZE)) { 
		    /* search [L, L+BLOCK_SIZE) */ 
		    mram_read((__mram_ptr void const *)L, cache_A, BLOCK_SIZE); 
		    DTYPE f = search(cache_A, searching_for, BLOCK_SIZE); 
		    if (f > -1) { 
			    result->found = f + (DTYPE)((L - A_base) / sizeof(DTYPE)); 
		    } else { 
			    /* search (L+BLOCK_SIZE, R) remainder */ 
			    const uint32_t start2 = L + BLOCK_SIZE; 
			    size_t remain = (size_t)(R - start2); 
			    remain &= ~((size_t)7); 
			    if (remain) { 
				    mram_read((__mram_ptr void const *)start2, cache_A, remain); 
				    f = search(cache_A, searching_for, remain); 
				    if (f > -1) 
					    result->found = f + (DTYPE)((start2 - A_base) / sizeof(DTYPE)); 
#if PR 
				    else 
					    printf("%lld NOT found\n", (long long)searching_for); 
#endif 
			    } 
		    } 
		    break; 
	    }
     // Load cache with current MRAM block
           mram_read((__mram_ptr void const *)mid, cache_A, BLOCK_SIZE);
           DTYPE f = search(cache_A, searching_for, BLOCK_SIZE);
           if (f > -1) {
               result->found = f + (DTYPE)((mid - A_base) / sizeof(DTYPE));
               break;
           }
           if (f == -2) {
               R = mid;          /* discard right */
           } else {              /* f == -1 */
               L = mid;          /* discard left  */
           }
       }
    }
    mybarrier_wait();

    /* publish per-DPU max(found) to host (8B) */
    if (tasklet_id == 0) {
        DTYPE maxf = (DTYPE)-1;
        for (unsigned t = 0; t < NR_TASKLETS; t++)
            if (DPU_RESULTS[t].found > maxf) maxf = DPU_RESULTS[t].found;
        sk_log_write_idx(0, (uint64_t)maxf);
    	__ime_wait_for_host();
    }
  return 0;
}
