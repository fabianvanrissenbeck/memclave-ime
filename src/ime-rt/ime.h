#ifndef IME_H
#define IME_H

#include <mram.h>
#include <stdint.h>

extern void __ime_wait_for_host(void);

extern void __ime_replace_sk(void __mram_ptr* sk, const uint8_t tag[16], const uint8_t user_key[32], uint32_t wipe_wram);

extern void __ime_get_counter(uint32_t out[4]);

#endif
