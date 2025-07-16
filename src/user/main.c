#include "poly.h"

#include <mram.h>
#include <defs.h>
#include <assert.h>

#define OUTPUT ((uint32_t __mram_ptr*) ((64 << 20) - 64))

const uint32_t input[8] = {
    0x2ae6eb98, 0x3ecce531, 0xab009248, 0x8b840bc5,
    0x6c6c6548, 0x57202c6f, 0x646c726f, 0x0000002e,
};

const uint32_t iv[3] = {
    0x00000000, 0x03020100, 0x07060504
};

extern void ime_chacha_blk(uint32_t c, uint32_t iv_a, uint32_t iv_b, uint32_t iv_c);
extern uint32_t chacha_output[16];

int main(void) {
    poly_context ctx;
    uint32_t tag[4];

    ime_chacha_blk(0, iv[0], iv[1], iv[2]);
    poly_init(&ctx, chacha_output);

    poly_feed_block(&ctx, &input[4]);
    poly_feed_block(&ctx, (uint32_t[4]) { 16, 0x0, 0x0, 0x0 });

    poly_finalize(&ctx, tag);

    for (int i = 0; i < 4; ++i) {
        OUTPUT[i] = tag[i];
        OUTPUT[i + 4] = input[i];
    }

    asm("stop");
    return 0;
}