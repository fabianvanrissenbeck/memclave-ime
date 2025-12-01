#ifndef IME_H
#define IME_H

#include <mram.h>
#include <stdint.h>

#define __ime_persist __attribute__((section(".data.persist")))

/**
 * Properties stored at the end of the persistent data storage.
 * These are erased by the rt library when returning from main.
 * They are not erased, if subkernels call __ime_replace_sk
 * directly.
 */
typedef struct {
    uint32_t tag[4];
    uint32_t key[8];
} ime_load_params;

extern __attribute__((section(".data.persist.last"))) ime_load_params g_load_prop;

#pragma GCC push
#pragma GCC diagnostic ignored "-Wignored-attributes"
extern uint32_t __mram_noinit __ime_debug_out[16];
#pragma GCC pop

extern void __ime_wait_for_host(void);

extern void __ime_replace_sk(void __mram_ptr* sk, const uint32_t tag[4], const uint32_t user_key[8]);

extern void __ime_get_counter(uint32_t out[4]);

extern void __ime_chacha_blk(const uint32_t key[8], uint32_t c, uint32_t iv_a, uint32_t iv_b, uint32_t iv_c, uint32_t out[16]);

#endif
