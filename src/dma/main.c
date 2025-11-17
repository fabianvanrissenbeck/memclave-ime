#include <ime.h>
#include <mram.h>
#include <stdio.h>
#include <perfcounter.h>

#pragma GCC push
#pragma GCC diagnostic ignored "-Wignored-attributes"
extern uint64_t __mram_noinit __ime_debug_out[8];
#pragma GCC pop

uint64_t __mram_noinit buffer[(16 << 20) / sizeof(uint64_t)];
static uint64_t wram_buf[2048 / sizeof(uint64_t)];

int main(void) {
    perfcounter_config(COUNT_CYCLES, true);

    for (int i = 0; i < (16 << 20); i += 64) {
        mram_read(&buffer[i / sizeof(uint64_t)], &wram_buf[0], 64);
    }

    // printf("CYCLES: %llu\n", perfcounter_get());
    __ime_debug_out[0] = perfcounter_get();
}
