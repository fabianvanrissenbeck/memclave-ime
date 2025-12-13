/*
* Histogram (HST-L) with multiple tasklets
*
*/
#include <stdint.h>
#include <stdio.h>
#include <defs.h>
#include <mram.h>
#include <alloc.h>
#include <perfcounter.h>
#include <barrier.h>
#include <atomic_bit.h>
#include <mutex.h>
#include <atomic_bit.h>

#include "support/common.h"
#include "support/log.h"
//#include "support/memclave_mutex.h"

#define NR_TASKLETS 16
#define ARG_OFFSET  0x2000
#define ARG_SIZE    sizeof(dpu_arguments_t)
#define A_OFFSET    (ARG_OFFSET + ((ARG_SIZE + 0xFF) & ~0xFF))

//static mc_mutex_pool_t g_mutexes;

typedef struct { volatile uint32_t v; uint32_t pad; } barrier_slot_t;
__attribute__((aligned(8)))
static struct { barrier_slot_t arrive[NR_TASKLETS]; volatile uint32_t sense; } gbar;

static inline void mybarrier_init(void){
  if (me()==0){ gbar.sense=0; for(uint32_t i=0;i<NR_TASKLETS;i++) gbar.arrive[i].v=1; __asm__ __volatile__("":::"memory"); }
  while (gbar.sense!=0){ __asm__ __volatile__("":::"memory"); }
}
static inline void mybarrier_wait(void){
  const uint32_t tid = me(); const uint32_t next = !gbar.sense;
  gbar.arrive[tid].v = next; __asm__ __volatile__("":::"memory");
  if (tid==0){ for(uint32_t i=0;i<NR_TASKLETS;i++) while (gbar.arrive[i].v!=next){ __asm__ __volatile__("":::"memory"); }
               gbar.sense = next; __asm__ __volatile__("":::"memory"); }
  else        { while (gbar.sense!=next){ __asm__ __volatile__("":::"memory"); } }
}

static inline __attribute__((always_inline))
uint32_t map_bin(T d, uint32_t bins){ return (uint32_t)(((uint32_t)d * bins) >> DEPTH); }

int main(){
  const uint32_t tid = me();
  if (tid==0){ 
	  mybarrier_init(); 
	  mem_reset(); 
	  sk_log_init(); 
	  //mc_mutex_init(&g_mutexes);
          //histo_dpu = (uint32_t *)mem_alloc(bins * sizeof(uint32_t));
          //for (uint32_t i = 0; i < bins; i++) histo_dpu[i] = 0;
  }
  mybarrier_wait();

  dpu_arguments_t args;
  mram_read((__mram_ptr void const*)ARG_OFFSET, &args, sizeof(args));
  const uint32_t input_bytes = args.size;
  const uint32_t xfer_bytes  = args.transfer_size;
  const uint32_t bins        = args.bins;

  const uint32_t mram_A = (uint32_t)A_OFFSET;
  const uint32_t mram_H = (uint32_t)(A_OFFSET + xfer_bytes);

  // Shared WRAM histogram (written in serialized flush)
  static uint32_t *histo_dpu;
  if (tid==0){
    histo_dpu = (uint32_t*)mem_alloc(bins * sizeof(uint32_t));
    for (uint32_t i=0;i<bins;i++) histo_dpu[i]=0;
  }
  mybarrier_wait();

  // ---- Local per-tasklet grouped counters ----
  // Choose a power-of-two number of "locks" (groups) to keep the groups tiny.
  #define NR_GROUPS 32u
  const uint32_t group_sz = (bins + NR_GROUPS - 1) / NR_GROUPS;   // ceil
  // total local counters = NR_GROUPS * group_sz ~= bins
  uint32_t *local = (uint32_t*)mem_alloc(NR_GROUPS * group_sz * sizeof(uint32_t));
  for (uint32_t i=0;i<NR_GROUPS*group_sz;i++) local[i]=0;

  // Scratch block for MRAM reads
  T *cache_A = (T*)mem_alloc(BLOCK_SIZE);

  // Strided scan
  const uint32_t base = tid << BLOCK_SIZE_LOG2;
  for (uint32_t byte_idx = base; byte_idx < input_bytes; byte_idx += BLOCK_SIZE * NR_TASKLETS){
    const uint32_t l_bytes = (byte_idx + BLOCK_SIZE >= input_bytes) ? (input_bytes - byte_idx) : BLOCK_SIZE;
    mram_read((const __mram_ptr void*)(mram_A + byte_idx), cache_A, l_bytes);
    const uint32_t l_elems = l_bytes >> DIV;    // DIV = log2(sizeof(T))

#pragma unroll
    for (uint32_t j=0;j<l_elems;j++){
      const uint32_t b = map_bin(cache_A[j], bins);
#if 0
      const uint32_t lid = mc_lock_for_bin(b);
      mc_mutex_lock(&g_mutexes, lid, (uint16_t)tid);
      histo_dpu[b] += 1;
      mc_mutex_unlock(&g_mutexes, lid, (uint16_t)tid);
#else
      const uint32_t g = b / group_sz;         // group id
      const uint32_t k = b - g*group_sz;       // index within group
      local[g*group_sz + k] += 1;
#endif
    }
  }

  // ---- Serialized flush by group and by tasklet turn ----
  for (uint32_t g=0; g<NR_GROUPS; g++){
    const uint32_t lo = g * group_sz;
    uint32_t hi = lo + group_sz; if (hi > bins) hi = bins;
    if (lo >= hi) continue;

    for (uint32_t turn=0; turn<NR_TASKLETS; turn++){
      mybarrier_wait();
      if (tid == turn){
        // add this tasklet's local group slice to shared histogram
        for (uint32_t b = lo; b < hi; b++){
          histo_dpu[b] += local[g*group_sz + (b - lo)];
        }
      }
    }
    mybarrier_wait();
  }

  // Final write to MRAM (single tasklet)
  mybarrier_wait();
  if (tid==0){
    const uint32_t bytes = bins * sizeof(uint32_t);
    if (bytes <= 2048) {
      mram_write(histo_dpu, (__mram_ptr void*)mram_H, bytes);
    } else {
      uint32_t off=0;
      while (off < bytes){
        const uint32_t chunk = ((bytes - off) > 2048) ? 2048 : (bytes - off);
        mram_write((void*)((uint8_t*)histo_dpu + off), (__mram_ptr void*)(mram_H + off), chunk);
        off += chunk;
      }
    }
    // Optional: sum for debugging
    uint64_t sum = 0; for (uint32_t i=0;i<bins;i++) sum += histo_dpu[i];
    sk_log_write_idx(0, 0xffff);
    sk_log_write_idx(1, bins);
    sk_log_write_idx(2, args.size);
    sk_log_write_idx(3, xfer_bytes);
    sk_log_write_idx(4, sum);
  }
  return 0;
}
