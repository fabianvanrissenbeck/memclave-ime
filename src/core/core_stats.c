#include "core.h"

#include <mram.h>
#include <perfcounter.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
extern uint64_t __mram_noinit __ime_debug_out[8];
#pragma GCC diagnostic pop

void ime_init_stats(void) {
    uint64_t index = 0;

    mram_write(&index, &__ime_debug_out[0], sizeof(index));
    perfcounter_config(COUNT_CYCLES, true);
}

void ime_stats_here(void) {
    uint64_t index;
    uint64_t count;

    count = perfcounter_get();
    mram_read(&__ime_debug_out[0], &index, sizeof(index));
    index += 1;

    mram_write(&count, &__ime_debug_out[index], sizeof(count));
    mram_write(&index, &__ime_debug_out[0], sizeof(index));
}
