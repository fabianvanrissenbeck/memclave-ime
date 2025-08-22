#include "poly.h"

#include <mram.h>
#include <string.h>
#include <assert.h>
#include <perfcounter.h>

extern uint64_t __mram_noinit __ime_debug_out[8];

const uint32_t input[] = {
    0xb7b71824, 0x9a505483, 0xa00e6f7f, 0x64a0ff30,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000010, 0x00000000, 0x00000000, 0x00000000,
};

const uint32_t key[] = {
    0x8ba0d58a, 0xcc815f90, 0x27405081, 0x7194b24a,
    0x37b633a8, 0xa50dfde3, 0xe2b8db08, 0x46a6d1fd,
};

int main(void) {
    poly_context ctx;
    uint32_t tag[4];

    poly_init(&ctx, key);
    perfcounter_config(COUNT_CYCLES, true);
    poly_feed_block(&ctx, &input[4]);
    __ime_debug_out[0] = perfcounter_get();
    poly_feed_block(&ctx, &input[8]);
    poly_finalize(&ctx, &tag[0]);

    assert(memcmp(&input[0], &tag[0], sizeof(tag)) == 0);
    asm("stop");
}
