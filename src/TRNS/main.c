/*
* 3-step matrix transposition with multiple tasklets
* Acks: Stefano Ballarin (P&S PIM Fall 2020)
*
*/
#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <mram.h>
#include <alloc.h>
#include <perfcounter.h>
#include <mutex.h>
#include <barrier.h>

#define NR_TASKLETS 16
#include "support/common.h"
#include "support/log.h"
#include "support/mc_sync.h"

#define ARG_OFFSET  0x2000u
#define ARG_SIZE    ((uint32_t)sizeof(dpu_arguments_t))
#define A_OFFSET    (ARG_OFFSET + ((ARG_SIZE + 0xFFu) & ~0xFFu))

/* Read args (8B aligned) */
static inline __attribute__((always_inline))
void read_args_aligned(dpu_arguments_t *args) {
    _Alignas(8) uint8_t buf[(sizeof(dpu_arguments_t) + 7u) & ~7u];
    mram_read((__mram_ptr void const*)ARG_OFFSET, buf, sizeof(buf));
    __builtin_memcpy(args, buf, sizeof(dpu_arguments_t));
}

uint32_t curr_tile = 0; // protected by MUTEX
uint32_t get_tile();
void read_tile_step2(uint32_t A, uint32_t offset, T* variable, uint32_t m, uint32_t n);
void write_tile_step2(uint32_t A, uint32_t offset, T* variable, uint32_t m, uint32_t n);
void read_tile_step3(uint32_t A, uint32_t offset, T* variable, uint32_t m);
void write_tile_step3(uint32_t A, uint32_t offset, T* variable, uint32_t m);
_Bool get_done(uint32_t done_array_step3, uint32_t address, T* read_done);
_Bool get_and_set_done(uint32_t done_array_step3, uint32_t address, T* read_done);

// Mutexes
MUTEX_INIT(tile_mutex);
MUTEX_INIT(done_mutex);

extern int main_kernel1(void);
extern int main_kernel2(void);

int (*kernels[nr_kernels])(void) = {main_kernel1, main_kernel2};

int main(void) { 
    const uint32_t tid = me();
    dpu_arguments_t args;
    read_args_aligned(&args);

    if (tid==0){ 
            mybarrier_init(); 
            mem_reset(); 
            sk_log_init(); 
    }
    mybarrier_wait();
    // Kernel
    return kernels[args.kernel]();
}

// Step 2: 0010
int main_kernel1() {
    unsigned int tasklet_id = me();
#if PRINT
    printf("tasklet_id = %u\n", tasklet_id);
#endif
    // Barrier
    mybarrier_wait();

    dpu_arguments_t args;
    read_args_aligned(&args);
    const uint32_t A   = (uint32_t)A_OFFSET;   // base of A in MRAM
    const uint32_t M_  = args.M_;
    const uint32_t m   = args.m;
    const uint32_t n   = args.n;

    T* data = (T*) mem_alloc(m * n * sizeof(T));
    T* backup = (T*) mem_alloc(m * n * sizeof(T));

    for(unsigned int tile = tasklet_id; tile < M_; tile += NR_TASKLETS){
        read_tile_step2(A, tile * m * n, data, m, n);
        for (unsigned int i = 0; i < m * n; i++){
            backup[(i * m) - (m * n - 1) * (i / n)] = data[i];
        }
        write_tile_step2(A, tile * m * n, backup, m, n);
    }

    return 0;
}

// Step 3: 0100
int main_kernel2() {
    unsigned int tasklet_id = me();
#if PRINT
    printf("tasklet_id = %u\n", tasklet_id);
#endif
    // Barrier
    mybarrier_wait();

    dpu_arguments_t args;
    read_args_aligned(&args);

    const uint32_t A          = (uint32_t)A_OFFSET;
    const uint32_t m          = args.m;
    const uint32_t n          = args.n;
    const uint32_t M_         = args.M_;
    const uint32_t tile_max   = M_ * n - 1u;
    const uint32_t done_array = (uint32_t)(A_OFFSET + (uint32_t)(M_ * m * n) * (uint32_t)sizeof(T));

    T* data = (T*)mem_alloc(sizeof(T) * m);
    T* backup = (T*)mem_alloc(sizeof(T) * m);
    T* read_done = (T*)mem_alloc(sizeof(T));

    uint32_t tile;
    _Bool done;

    tile = get_tile();

    while (tile < tile_max){
        uint32_t next_in_cycle = ((tile * M_) - tile_max * (tile / n));
        if (next_in_cycle == tile){
            tile = get_tile();
            continue;
        }
        read_tile_step3(A, tile * m, data, m);

        done = get_done(done_array, tile, read_done);
        for(; done == 0; next_in_cycle = ((next_in_cycle * M_) - tile_max * (next_in_cycle / n))){
            read_tile_step3(A, next_in_cycle * m, backup, m);

            done = get_and_set_done(done_array, next_in_cycle, read_done);

            if(!done) {
                write_tile_step3(A, next_in_cycle * m, data, m);
            }
            for(uint32_t i = 0; i < m; i++){
                data[i] = backup[i];
            }
        }
        tile = get_tile();
    }
		
    return 0;
}

// Auxiliary functions
uint32_t get_tile(){
    mutex_lock(tile_mutex);
    uint32_t value = curr_tile;
    curr_tile++;
    mutex_unlock(tile_mutex);
    return value;
}

void read_tile_step2(uint32_t A, uint32_t offset, T* variable, uint32_t m, uint32_t n){
    int rest = m * n;
    int transfer;
    while(rest > 0){
        if(rest * sizeof(T) > 2048){
            transfer = 2048 / sizeof(T);
      } else {
            transfer = rest;
      }
      //mram_read((__mram_ptr void*)(A + (offset + m * n - rest) * sizeof(T)), variable + (m * n - rest) * sizeof(T), sizeof(T) * transfer);
      mram_read((__mram_ptr void*)(A + (offset + m * n - rest) * sizeof(T)), variable + (m * n - rest), sizeof(T) * transfer);
      rest -= transfer;
    }
}

void write_tile_step2(uint32_t A, uint32_t offset, T* variable, uint32_t m, uint32_t n){
    int rest = m * n;
    int transfer;
    while(rest > 0){
        if(rest * sizeof(T) > 2048){
            transfer = 2048 / sizeof(T);
      } else {
            transfer = rest;
      }
      //mram_write(variable + (m * n - rest) * sizeof(T), (__mram_ptr void*)(A + (offset + m * n - rest) * sizeof(T)), sizeof(T) * transfer);
      mram_write(variable + (m * n - rest) * sizeof(T), (__mram_ptr void*)(A + (offset + m * n - rest)), sizeof(T) * transfer);
      rest -= transfer;
    }
}

void read_tile_step3(uint32_t A, uint32_t offset, T* variable, uint32_t m){
    mram_read((__mram_ptr void*)(A + offset * sizeof(T)), variable, sizeof(T) * m);
}

void write_tile_step3(uint32_t A, uint32_t offset, T* variable, uint32_t m){
    mram_write(variable, (__mram_ptr void*)(A + offset * sizeof(T)), sizeof(T) * m);
}

_Bool get_done(uint32_t done_array_step3, uint32_t address, T* read_done){
    uint32_t result;

    mutex_lock(done_mutex);
    mram_read((__mram_ptr void*)(done_array_step3 + address), read_done, sizeof(T));
    result = ((*read_done & (0x01 << (address % sizeof(T)))) != 0);
    mutex_unlock(done_mutex);

    return (_Bool)result;
}

_Bool get_and_set_done(uint32_t done_array_step3, uint32_t address, T* read_done){
    uint32_t result;

    mutex_lock(done_mutex);
    mram_read((__mram_ptr void*)(done_array_step3 + address), read_done, sizeof(T));
    result = ((*read_done & (0x01 << (address % sizeof(T)))) != 0);
    *read_done |= (0x01 << (address % sizeof(T)));
    mram_write(read_done, (__mram_ptr void*)(done_array_step3 + address), sizeof(T));
    mutex_unlock(done_mutex);

    return (_Bool)result;
}
