#pragma once
#include <stdint.h>

#ifndef NR_TASKLETS
#define NR_TASKLETS 16
#endif
#ifndef MC_NR_LOCKS
#define MC_NR_LOCKS 32u   /* power-of-two is convenient for bin→lock mapping */
#endif

typedef struct {
  volatile uint16_t choosing[NR_TASKLETS];
  volatile uint16_t number[NR_TASKLETS];
} mc_bakery_t;

typedef struct {
  mc_bakery_t L[MC_NR_LOCKS];
} mc_mutex_pool_t;

static inline void mc_mutex_init(mc_mutex_pool_t *pool) {
  for (uint32_t g = 0; g < MC_NR_LOCKS; g++) {
    for (uint32_t t = 0; t < NR_TASKLETS; t++) {
      pool->choosing[g].choosing[t] = 0;
      pool->L[g].number[t] = 0;
    }
  }
  __asm__ __volatile__("" ::: "memory");
}

static inline uint16_t mc_max_ticket(const mc_bakery_t *B) {
  uint16_t m = 0;
  for (uint16_t t = 0; t < NR_TASKLETS; t++) {
    uint16_t v = B->number[t];
    if (v > m) m = v;
  }
  return m;
}

static inline void mc_mutex_lock(mc_mutex_pool_t *pool, uint32_t g, uint16_t tid) {
  mc_bakery_t *B = &pool->L[g];
  B->choosing[tid] = 1;                    __asm__ __volatile__("" ::: "memory");
  uint16_t my = (uint16_t)(mc_max_ticket(B) + 1);
  B->number[tid]  = my;                    __asm__ __volatile__("" ::: "memory");
  B->choosing[tid]= 0;                     __asm__ __volatile__("" ::: "memory");

  for (uint16_t k = 0; k < NR_TASKLETS; k++) {
    if (k == tid) continue;
    while (B->choosing[k]) { __asm__ __volatile__("" ::: "memory"); }
    for (;;) {
      uint16_t nk = B->number[k];
      if (nk == 0) break;
      /* (nk, k) (my, tid) ? then we must wait */
      if ((nk < my) || (nk == my && k < tid)) {
        __asm__ __volatile__("" ::: "memory");
      } else break;
    }
  }
  __asm__ __volatile__("" ::: "memory");   /* acquire */
}

static inline void mc_mutex_unlock(mc_mutex_pool_t *pool, uint32_t g, uint16_t tid) {
  __asm__ __volatile__("" ::: "memory");   /* release */
  pool->L[g].number[tid] = 0;              __asm__ __volatile__("" ::: "memory");
}

static inline uint32_t mc_lock_for_bin(uint32_t b) { return (b & (MC_NR_LOCKS - 1)); }
