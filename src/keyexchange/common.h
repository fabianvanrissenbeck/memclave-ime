#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

struct ime_xchg_io {
    union {
        struct {
            uint32_t client_pk_in[64];
            uint32_t xchg_cnt_in[4];
            // use OTP for now - AEAD would be cleaner though
            uint32_t key_enc_in[12];
        } in;
        struct {
            uint32_t xchg_shared_out[64];
            uint32_t xchg_cnt_out[4];
            uint32_t key_enc_out[12];
        } out;
    };
};

#pragma GCC push
#pragma GCC diagnostic ignored "-Wignored-attributes"
extern uint64_t __mram_noinit __ime_msg_sk[];
#pragma GCC pop


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
