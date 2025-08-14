#include <ime.h>
#include <defs.h>
#include <mutex.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <perfcounter.h>

MUTEX_INIT(g_start_lock);
MUTEX_INIT(g_finished_lock);

volatile uint32_t g_start_ctr = 0;
volatile uint32_t g_finished_ctr = 0;

#define DECRYPT_BUFFER_SIZE (16 << 20)

#pragma GCC push
#pragma GCC diagnostic ignored "-Wignored-attributes"
extern uint64_t __mram_noinit __ime_debug_out[8];
#pragma GCC pop

uint64_t __mram_noinit input_buffer_enc[DECRYPT_BUFFER_SIZE / sizeof(uint64_t)];

uint32_t key[8] = {
    0x83828180, 0x87868584, 0x8b8a8988, 0x8f8e8d8c,
    0x93929190, 0x97969594, 0x9b9a9998, 0x9f9e9d9c,
};

uint32_t target_mat[16] = {
    0x8ba0d58a, 0xcc815f90, 0x27405081, 0x7194b24a,
    0x37b633a8, 0xa50dfde3, 0xe2b8db08, 0x46a6d1fd,
    0x7da03782, 0x9183a233, 0x148ad271, 0xb46773d1,
    0x3cc1875a, 0x8607def1, 0xca5c3086, 0x7085eb87
};

__attribute__((aligned(8)))
uint32_t mat[NR_TASKLETS][16];

__attribute__((aligned(8)))
uint32_t buf[NR_TASKLETS][16];

int main(void) {
    if (me() == 0) {
        perfcounter_config(COUNT_CYCLES, true);
    }

    mutex_lock(g_start_lock);
    g_start_ctr++;
    mutex_unlock(g_start_lock);

    if (me() == 0) {
        while (g_start_ctr < NR_TASKLETS) {}
    }

    for (size_t i = me(); i < DECRYPT_BUFFER_SIZE / 64; i += NR_TASKLETS) {
        uint64_t __mram_ptr* block = &input_buffer_enc[i * 64 / sizeof(uint64_t)];
        uint32_t* b = &buf[me()][0];
        uint32_t* m = &mat[me()][0];

        mram_read(block, b, 64);
        __ime_chacha_blk(key, 0, 0x00000000, 0x03020100, 0x07060504, m);

#if 1
#pragma loop unroll 16
        for (size_t j = 0; j < 16; ++j) {
            b[j] ^= m[j];
        }
#endif

#if 1
        mram_write(b, block, 64);
#endif
    }

    mutex_lock(g_finished_lock);
    g_finished_ctr++;
    mutex_unlock(g_finished_lock);

    if (me() == 0) {
        while (g_finished_ctr < NR_TASKLETS) {}
        __ime_debug_out[0] = perfcounter_get();
    }
}
