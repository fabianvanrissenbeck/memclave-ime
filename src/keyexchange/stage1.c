#include "ime.h"
#include "common.h"

#include <stddef.h>

__attribute__((section(".data.persist")))
struct ime_xchg_io g_xchg_io;

volatile uint32_t __mram_noinit client_pubkey_raw[64];
volatile uint32_t __mram_noinit dpu_pubkey_raw[64];
volatile uint32_t __mram_noinit ime_xchg_counter[4];

static uint32_t dpu_pubkey_wram[64] = {
    0x37a59d0c, 0x4c654ae0, 0x54204efc, 0x90130854,
    0x4ec76607, 0xf6b1f615, 0x40c18133, 0xe9ea753c,
    0x52ed11fc, 0xe979bf70, 0xdc9ecf89, 0xe0e2465b,
    0xe118e4b0, 0x7cf7bb2a, 0xa2da44c1, 0xa62b132d,
    0xc234275e, 0x716ebe7a, 0x8d84c6e7, 0x7e3cfee9,
    0x88356cb3, 0x4e8656f6, 0x16078c7e, 0x2a6a85dd,
    0xcdbfd286, 0x15647358, 0xa0ab2396, 0x94ac675a,
    0x9cafbd03, 0xc71cf41b, 0x8fb89127, 0x7df7c596,
    0xbd3615f9, 0x912600e2, 0x60a6007f, 0x5d340b2,
    0xb22cbd82, 0xfa5ab325, 0xb4dff05f, 0x1a0bda46,
    0x3ca6e1cf, 0xf7defcbc, 0x66037e91, 0x670d35d5,
    0xfe4f59c1, 0x7accaccb, 0x64543931, 0x427a5ee7,
    0x7a11ffd2, 0x5b9e3278, 0x1516b8c5, 0x876eb704,
    0xf3376885, 0x5b742454, 0x22ea5ad3, 0xeef43223,
    0xc36f1b07, 0x526ca290, 0x4175e164, 0x8c83f994,
    0x65d1a0f4, 0xdac3834b, 0x31017203, 0x2404b340
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
        g_xchg_io.in.xchg_cnt_in[i] = ctr[i];
    }

    for (int i = 0; i < 64; ++i) {
        g_xchg_io.in.client_pk_in[i] = client_pubkey_raw[i];
    }

    __ime_replace_sk(__ime_xchg_sk_2, NULL, NULL);
    __builtin_unreachable();
}
