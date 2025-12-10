#include "ime.h"
#include "aead.h"
#include "barrier.h"

#include <defs.h>
#include <mram.h>
#include <assert.h>
#include <perfcounter.h>

// actual size is calculated by 1 << size
#define MIN_BLOCK_SIZE 6
#define MAX_BLOCK_SIZE 24

IME_BARRIER_INIT(bench_barrier, NR_TASKLETS)

__mram_keep uint64_t out_time_enc[MAX_BLOCK_SIZE - MIN_BLOCK_SIZE + 1];
__mram_keep uint64_t out_time_dec[MAX_BLOCK_SIZE - MIN_BLOCK_SIZE + 1];

__attribute__((aligned(8)))
__mram_keep uint32_t buffer[(1 << MAX_BLOCK_SIZE) / sizeof(uint32_t)];

static uint32_t key[8] = {
    0x4ae93ca1, 0x017a43d2, 0x9942e9a2, 0xf2ce2f1d,
    0x4dbbf9d4, 0x3996fd36, 0xadfbdced, 0xbadcf4d8
};

static uint32_t iv[3] = { 0 };

static void wipe_buffer_content(size_t sz) {
    uint64_t zero = 0;

    for (size_t i = 0; i < sz / sizeof(buffer[0]); i += 2) {
        mram_write(&zero, &buffer[i], sizeof(zero));
    }
}

int main(void) {
    for (unsigned bs = MIN_BLOCK_SIZE; bs <= MAX_BLOCK_SIZE; bs += 1) {
        size_t sz = 1 << bs;
        perfcounter_t tm_enc = 0;
        perfcounter_t tm_dec = 0;

        if (me() == 0) {
            wipe_buffer_content(sz);
        }

        ime_barrier_wait(&bench_barrier);

        if (me() == 0) {
            perfcounter_config(COUNT_CYCLES, true);
        }

        ime_chacha_enc_mram(&key[0], &iv[0], NR_TASKLETS, sz, &buffer[0], &buffer[0], NULL);
        ime_barrier_wait(&bench_barrier);

        if (me() == 0) {
            tm_enc = perfcounter_get();
            perfcounter_config(COUNT_CYCLES, true);
        }

        ime_chacha_dec_mram(&key[0], &iv[0], NR_TASKLETS, sz, &buffer[0], &buffer[0]);
        ime_barrier_wait(&bench_barrier);

        if (me() == 0) {
            tm_dec = perfcounter_get();
            out_time_enc[bs - MIN_BLOCK_SIZE] = tm_enc;
            out_time_dec[bs - MIN_BLOCK_SIZE] = tm_dec;
        }
    }
}
