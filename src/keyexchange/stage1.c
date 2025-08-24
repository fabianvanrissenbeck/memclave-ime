#include "ime.h"
#include "common.h"

#include <stddef.h>

__attribute__((section(".data.persist")))
struct ime_xchg_io g_xchg_io;

volatile uint32_t __mram_noinit client_pubkey_raw[64];
volatile uint32_t __mram_noinit dpu_pubkey_raw[64];
volatile uint32_t __mram_noinit ime_xchg_counter[4];

static uint32_t dpu_pubkey_wram[64] = {
    0x123
};

int main(void) {
    uint32_t ctr[4];
    __ime_get_counter(&ctr[0]);

    for (int i = 0; i < 4; ++i) {
        ime_xchg_counter[i] = ctr[i];
    }

    for (int i = 0; i < 64; ++i) {
        dpu_pubkey_raw[i] = dpu_pubkey_wram[i];
    }

    __ime_wait_for_host(); // wait for host to share public key

    for (int i = 0; i < 4; ++i) {
#if 0
        g_xchg_io.in.xchg_cnt_in[i] = ctr[i];
#else
        g_xchg_io.in.xchg_cnt_in[i] = 0;
#endif
    }

    for (int i = 0; i < 64; ++i) {
#if 0
        g_xchg_io.in.client_pk_in[i] = client_pubkey_raw[i];
#else
        g_xchg_io.in.client_pk_in[i] = (i == 0) ? 2 : 0;
#endif
    }

    __ime_replace_sk(__ime_xchg_sk_2, NULL, NULL);
    __builtin_unreachable();
}
