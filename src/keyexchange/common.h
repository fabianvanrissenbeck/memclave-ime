#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

struct ime_xchg_io {
    union {
        struct {
            uint32_t client_pk_in[64];
            uint32_t xchg_cnt_in[4];
        } in;
        struct {
            uint32_t xchg_shared_out[64];
            uint32_t xchg_cnt_out[4];
        } out;
    };
};

#pragma GCC push
#pragma GCC diagnostic ignored "-Wignored-attributes"
extern uint64_t __mram_noinit __ime_xchg_sk_2[];
#pragma GCC pop

#pragma GCC push
#pragma GCC diagnostic ignored "-Wignored-attributes"
extern uint64_t __mram_noinit __ime_xchg_sk_3[];
#pragma GCC pop

__attribute__((section(".data.persist")))
extern struct ime_xchg_io g_xchg_io;

#endif
