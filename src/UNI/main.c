/*
* Unique with multiple tasklets
*
*/
#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <mram.h>
#include <alloc.h>
#include <perfcounter.h>
#include <handshake.h>
#include <barrier.h>

#define NR_TASKLETS 16

#include "support/common.h"
#include "support/mc_sync.h"
#include "support/log.h"       // sk_log_init / sk_log_write_idx

#define ARG_OFFSET  0x2000u
#define ARG_SIZE    ((uint32_t)sizeof(dpu_arguments_t))
#define A_OFFSET    (ARG_OFFSET + ((ARG_SIZE + 0xFFu) & ~0xFFu))

static inline __attribute__((always_inline))
void read_args_aligned(dpu_arguments_t *args) {
    _Alignas(8) uint8_t buf[(sizeof(dpu_arguments_t) + 7u) & ~7u];
    mram_read((__mram_ptr void const*)ARG_OFFSET, buf, sizeof(buf));
    __builtin_memcpy(args, buf, sizeof(dpu_arguments_t));
}

// Array for communication between adjacent tasklets
uint32_t message_partial_count;
T        message_last_from_last;

// UNI in each tasklet
static unsigned int unique(T *output, T *input){
    unsigned int pos = 0;
    output[pos] = input[pos];
    pos++;
    #pragma unroll
    for(unsigned int j = 1; j < REGS; j++) {
        if(input[j] != input[j - 1]) {
            output[pos] = input[j];
            pos++;
        }
    }
    return pos;
}

//typedef struct { uint32_t x, y, z; } uint3;
__attribute__((aligned(8))) static volatile uint32_t msg_count[NR_TASKLETS];
__attribute__((aligned(8))) static volatile T        msg_last[NR_TASKLETS];
__attribute__((aligned(8))) static volatile uint32_t msg_off[NR_TASKLETS];
__attribute__((aligned(8))) static volatile uint32_t msg_epoch[NR_TASKLETS];

/* one-round adjacency handshake */
static inline __attribute__((always_inline))
uint3 handshake_sync_uni(const T *out, unsigned l_count, unsigned tasklet_id)
{
    const uint32_t epoch = (uint32_t)g_epoch;

    uint32_t p_count = 0, o_count = 0, offset = 0;
    if (tasklet_id != 0) {
        while (msg_epoch[tasklet_id] != epoch) { __asm__ __volatile__("" ::: "memory"); }
        p_count = msg_count[tasklet_id];
        offset  = (msg_last[tasklet_id] == out[0]) ? 1u : 0u;
        o_count = msg_off[tasklet_id];
    } else {
        p_count = 0;
        offset  = (message_last_from_last == out[0]) ? 1u : 0u;
        o_count = 0;
    }

    if (tasklet_id < (NR_TASKLETS - 1)) {
        msg_count[tasklet_id + 1] = p_count + l_count;
        msg_last [tasklet_id + 1] = out[l_count - 1];
        msg_off  [tasklet_id + 1] = o_count + offset;
        __asm__ __volatile__("" ::: "memory");
        msg_epoch[tasklet_id + 1] = epoch;
        __asm__ __volatile__("" ::: "memory");
    }
    return (uint3){ p_count, o_count, offset };
}

extern int main_kernel1(void);

int (*kernels[nr_kernels])(void) = {main_kernel1};

int main(void) { 
    unsigned int tasklet_id = me();
    dpu_arguments_t args;
    read_args_aligned(&args);

    if (me() == 0) {
        mybarrier_init();
        handshake_init();   /* sets g_epoch=1 */
        sk_log_init();
        message_partial_count = 0;
        message_last_from_last = (T)~(T)0;
    }
    mybarrier_wait();

    const uint32_t input_size_dpu_bytes = args.size;

    // Address of the current processing block in MRAM
    uint32_t base_tasklet = tasklet_id << BLOCK_SIZE_LOG2;
    const uint32_t A_base = (uint32_t)A_OFFSET;
    const uint32_t B_base = (uint32_t)(A_OFFSET + input_size_dpu_bytes);

    // Initialize a local cache to store the MRAM block
    T *cache_A = (T *) mem_alloc(BLOCK_SIZE);
    T *cache_B = (T *) mem_alloc(BLOCK_SIZE);

    // Initialize shared variable
    if(tasklet_id == NR_TASKLETS - 1){
        message_partial_count = 0;
        message_last_from_last = 0xFFFFFFFF; // A value that is not in the input array
    }
    // Barrier
    mybarrier_wait();

    unsigned int i = 0; // Iteration count
    for(unsigned int byte_index = base_tasklet; byte_index < input_size_dpu_bytes; byte_index += BLOCK_SIZE * NR_TASKLETS){

        if (tasklet_id == 0) { 
		g_epoch++; 
		__asm__ __volatile__("" ::: "memory"); 
	}
        mybarrier_wait();

        // Load cache with current MRAM block
	mram_read((const __mram_ptr void *)(A_base + byte_index), cache_A, BLOCK_SIZE);

        // UNI in each tasklet
        unsigned int l_count = unique(cache_B, cache_A);

        // Sync with adjacent tasklets
	const uint3 po = handshake_sync_uni(cache_B, l_count, tasklet_id);

        // Write cache to current MRAM block
        if (l_count > po.z) {
            const uint32_t out_elems = l_count - po.z;
            mram_write(&cache_B[po.z],
                       (__mram_ptr void *)(B_base + (uint32_t)((message_partial_count + po.x - po.y) * sizeof(T))),
                       (uint32_t)(out_elems * sizeof(T)));
        }

        // Total count in this DPU
        if(tasklet_id == NR_TASKLETS - 1){
            const T last_val = cache_B[l_count ? (l_count - 1) : 0];
            message_last_from_last = last_val;
            /* total so far in this DPU after current block */
            message_partial_count = message_partial_count + po.x + l_count - po.y - po.z;
        }

        // Barrier
        mybarrier_wait();

        i++;
    }
    mybarrier_wait();
    if (tasklet_id == 0) {
        sk_log_write_idx(0, (uint64_t)message_partial_count);
    	__ime_wait_for_host();
    }

    return 0;
}
