/*
 * STREAMP implementation of Matrix Profile with multiple tasklets
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <mram.h>
#include <alloc.h>
#include <mram.h>
#include <barrier.h>

#define NR_TASKLETS 16

#include "support/common.h"
#define T DTYPE
#include "support/mc_sync.h"
#include "support/log.h"

#define ARG_OFFSET  0x2000u
#define ARG_SIZE    ((uint32_t)sizeof(dpu_arguments_t))
#define A_OFFSET    (ARG_OFFSET + ((ARG_SIZE + 0xFFu) & ~0xFFu))

#define DOTPIP      (BLOCK_SIZE / (uint32_t)sizeof(DTYPE))

static inline __attribute__((always_inline))
void read_args_aligned(dpu_arguments_t *args) {
    _Alignas(8) uint8_t buf[(sizeof(dpu_arguments_t) + 7u) & ~7u];
    mram_read((__mram_ptr void const*)ARG_OFFSET, buf, sizeof(buf));
    __builtin_memcpy(args, buf, sizeof(dpu_arguments_t));
}


//volatile dpu_arguments_t DPU_INPUT_ARGUMENTS;
volatile dpu_result_t DPU_RESULTS[NR_TASKLETS];

// Dot product
static void dot_product(DTYPE *vectorA, DTYPE *vectorA_aux, DTYPE *vectorB, DTYPE * result) {

	for(uint32_t i = 0; i <  BLOCK_SIZE / sizeof(DTYPE); i++)
	{
		for(uint32_t j = 0; j < DOTPIP; j++)
		{
			if((j + i) > BLOCK_SIZE / sizeof(DTYPE) - 1)
			{
				result[j] += vectorA_aux[(j + i) - BLOCK_SIZE / sizeof(DTYPE)]  * vectorB[i];
			}
			else
			{
				result[j] += vectorA[j + i] * vectorB[i];
			}
		}
	}
}

int main(void){
	unsigned int tasklet_id = me();
#if PRINT
	printf("tasklet_id = %u\n", tasklet_id);
#endif
	if(tasklet_id == 0){
            mem_reset();
            sk_log_init();
            mybarrier_init();
            for (unsigned t = 0; t < NR_TASKLETS; ++t) {
                DPU_RESULTS[t].minValue = DTYPE_MAX;
                DPU_RESULTS[t].minIndex = 0;
                DPU_RESULTS[t].maxValue = 0;
                DPU_RESULTS[t].maxIndex = 0;
            }
	}
	// Barrier
	mybarrier_wait();

        dpu_arguments_t args;
        read_args_aligned(&args);
	// Input arguments
        const uint32_t query_length  = (uint32_t)args.query_length;
        const DTYPE    query_mean    = (DTYPE)args.query_mean;
        const DTYPE    query_std     = (DTYPE)args.query_std;
        const uint32_t slice_per_dpu = (uint32_t)args.slice_per_dpu;

	// Boundaries for current tasklet
	uint32_t myStartElem = tasklet_id  * (slice_per_dpu / (NR_TASKLETS));
	uint32_t myEndElem   = myStartElem + (slice_per_dpu / (NR_TASKLETS)) - 1;

	// Check time series limit
	if(myEndElem > slice_per_dpu - query_length) myEndElem = slice_per_dpu - query_length;

	// Starting address of the current processing block in MRAM
        uint32_t mem_offset = (uint32_t)A_OFFSET;
        
        const uint32_t query_base = mem_offset;
        mem_offset += (uint32_t)(query_length * sizeof(DTYPE));
        
        const uint32_t ts_base = mem_offset;
        /* each tasklet starts at its own offset inside TS slice */
        mem_offset += (uint32_t)(myStartElem * sizeof(DTYPE));
        const uint32_t starting_offset_ts = mem_offset;
        
        /* mean/sigma bases (also shifted by myStartElem like PRIM code) */
        const uint32_t mean_base  = ts_base + (uint32_t)((slice_per_dpu + query_length) * sizeof(DTYPE));
        const uint32_t sigma_base = mean_base + (uint32_t)((slice_per_dpu + query_length) * sizeof(DTYPE));
        
        uint32_t current_mram_block_addr_TS      = starting_offset_ts;
        uint32_t current_mram_block_addr_TSMean  = mean_base  + (uint32_t)(myStartElem * sizeof(DTYPE));
        uint32_t current_mram_block_addr_TSSigma = sigma_base + (uint32_t)(myStartElem * sizeof(DTYPE));

	// Initialize local caches to store the MRAM blocks
	DTYPE *cache_TS       = (DTYPE *) mem_alloc(BLOCK_SIZE);
	DTYPE *cache_TS_aux   = (DTYPE *) mem_alloc(BLOCK_SIZE);
	DTYPE *cache_query    = (DTYPE *) mem_alloc(BLOCK_SIZE);
	DTYPE *cache_TSMean   = (DTYPE *) mem_alloc(BLOCK_SIZE);
	DTYPE *cache_TSSigma  = (DTYPE *) mem_alloc(BLOCK_SIZE);
	DTYPE *cache_dotprods = (DTYPE *) mem_alloc(BLOCK_SIZE);

	// Create result structure pointer
	//dpu_result_t *result = &DPU_RESULTS[tasklet_id];

	// Auxiliary variables
	DTYPE min_distance = DTYPE_MAX;
	uint32_t min_index = 0;


	for(uint32_t i = myStartElem; i < myEndElem; i+= (BLOCK_SIZE / sizeof(DTYPE)))
	{
		for(uint32_t d = 0; d < DOTPIP; d++)
			cache_dotprods[d] = 0;

		current_mram_block_addr_TS    = (uint32_t) starting_offset_ts + (i - myStartElem) * sizeof(DTYPE);
                uint32_t current_mram_block_addr_query = query_base;

		for(uint32_t j = 0; j < (query_length) / (BLOCK_SIZE / sizeof(DTYPE)); j++)
		{
			mram_read((__mram_ptr void const *) current_mram_block_addr_TS, cache_TS, BLOCK_SIZE);
			mram_read((__mram_ptr void const *) current_mram_block_addr_TS + BLOCK_SIZE, cache_TS_aux, BLOCK_SIZE);
			mram_read((__mram_ptr void const *) current_mram_block_addr_query, cache_query, BLOCK_SIZE);

			current_mram_block_addr_TS    += BLOCK_SIZE;
			current_mram_block_addr_query += BLOCK_SIZE;
			dot_product(cache_TS, cache_TS_aux, cache_query, cache_dotprods);
		}


		mram_read((__mram_ptr void const *) current_mram_block_addr_TSMean, cache_TSMean, BLOCK_SIZE);
		mram_read((__mram_ptr void const *) current_mram_block_addr_TSSigma, cache_TSSigma, BLOCK_SIZE);
		current_mram_block_addr_TSMean  += BLOCK_SIZE;
		current_mram_block_addr_TSSigma += BLOCK_SIZE;

		for (uint32_t k = 0; k < (BLOCK_SIZE / sizeof(DTYPE)); k++)
		{
			DTYPE distance = 2 * ((DTYPE) query_length - (cache_dotprods[k] - (DTYPE) query_length * cache_TSMean[k]
						* query_mean) / (cache_TSSigma[k] * query_std));

			if(distance < min_distance)
			{
				min_distance =  distance;
				min_index    =  i + k;
			}
		}
	}

        /* store per-tasklet result */
        DPU_RESULTS[tasklet_id].minValue = min_distance;
        DPU_RESULTS[tasklet_id].minIndex = min_index;
        
        mybarrier_wait();
        
        /* reduce + publish per-DPU min to MRAM log (2 words) */
        if (tasklet_id == 0) {
            DTYPE bestV = DTYPE_MAX;
            uint32_t bestI = 0;
        
            for (unsigned t = 0; t < NR_TASKLETS; t++) {
                DTYPE v = DPU_RESULTS[t].minValue;
                if (v > 0 && v < bestV) {
                    bestV = v;
                    bestI = (uint32_t)DPU_RESULTS[t].minIndex;
                }
            }
        
            sk_log_write_idx(0, (uint64_t)(int64_t)bestV);
            sk_log_write_idx(1, (uint64_t)bestI);
        }

	return 0;
}
