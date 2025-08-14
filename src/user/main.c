#include <ime.h>
#include <defs.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <perfcounter.h>

#define INPUT_BUFFER_SIZE (16 << 20)

#pragma GCC push
#pragma GCC diagnostic ignored "-Wignored-attributes"
extern uint64_t __mram_noinit __ime_debug_out[8];
#pragma GCC pop

uint64_t __mram_noinit input_buffer_enc[INPUT_BUFFER_SIZE >> 3];

uint32_t key[8] = {
    0x83828180, 0x87868584, 0x8b8a8988, 0x8f8e8d8c,
    0x93929190, 0x97969594, 0x9b9a9998, 0x9f9e9d9c,
};

int main(void) {
    uint32_t mat[16];
    uint32_t buf[16];

    if (me() == 0) {
        perfcounter_config(COUNT_CYCLES, true);
    }

    /*
     * TODO: There has to be a bug somewhere. Results for 16 MiB:
     *
     * NR_TASKLETS  CYCLES     SPEED
     *          1   0xc9218ba0 1.6595450216024874 MB/s
     *         16   0x05f3b3d0 56.079992501303856 MB/s (what)
     *         16*  n.a.       18.254995237627362 MB/s (expected speedup x11)
     */
    assert(me() < NR_TASKLETS);

    // decrypt 64 bytes per thread at once
    for (size_t i = me(); i < INPUT_BUFFER_SIZE / 64; i += NR_TASKLETS) {
        mram_read(&input_buffer_enc[i * 64 / sizeof(uint64_t)], &buf[0], sizeof(buf));
        __ime_chacha_blk(key, i + 1, 0x00000000, 0x03020100, 0x07060504, &mat[0]);

#if 1
#pragma GCC loop unroll 16
        for (int j = 0; j < 16; ++j) {
            buf[j] ^= mat[j];
        }

        if (me() == 12) {
            __ime_debug_out[3] = __ime_debug_out[1];
            __ime_debug_out[1] = i;
        }
#endif
    }

    if (me() == 0) {
        perfcounter_t perf = perfcounter_get();
        // __ime_debug_out[0] = perf * 1000 / 350000000;
        __ime_debug_out[0] = perf;
    }

    asm("stop");
}
