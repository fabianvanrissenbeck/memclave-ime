#include <stdint.h>
#include <assert.h>

extern uint32_t chacha_output[16];

extern void ime_chacha_blk(uint32_t c, uint32_t n_1, uint32_t n_2, uint32_t n_3);

uint32_t target[16] = {
    0xe4e7f110, 0x15593bd1, 0x1fdd0f50, 0xc47120a3,
    0xc7f4d1c7, 0x0368c033, 0x9aaa2204, 0x4e6cd4c3,
    0x466482d2, 0x09aa9f07, 0x05d7c214, 0xa2028bd9,
    0xd19c12b5, 0xb94e16de, 0xe883d0cb, 0x4e3c50a2
};

int main(void) {
    ime_chacha_blk(0x1, 0x09000000, 0x4a000000, 0x00000000);

    for (int i = 0; i < 16; i++) {
        assert(chacha_output[i] == target[i]);
    }

    asm("stop");
}