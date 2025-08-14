#ifndef IME_H
#define IME_H

#include <mram.h>
#include <stdint.h>

#define __ime_persist __attribute__((section(".data.persist")))

extern void __ime_wait_for_host(void);

extern void __ime_replace_sk(void __mram_ptr* sk, const uint32_t tag[4], const uint32_t user_key[8]);

extern void __ime_get_counter(uint32_t out[4]);

extern void __ime_chacha_blk(uint32_t key[8], uint32_t c, uint32_t iv_a, uint32_t iv_b, uint32_t iv_c, uint32_t out[16]);

#endif
