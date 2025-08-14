#include <ime.h>
#include <defs.h>
#include <mutex.h>
#include <stddef.h>
#include <perfcounter.h>

#define BUFFER_SIZE (16 << 10)
#define THREAD_SIZE BUFFER_SIZE / NR_TASKLETS / sizeof(uint64_t)

_Static_assert(THREAD_SIZE == 128, "oh no");

MUTEX_INIT(g_start_lock);
MUTEX_INIT(g_stop_lock);

#pragma GCC push
#pragma GCC diagnostic ignored "-Wignored-attributes"
extern uint64_t __mram_noinit __ime_debug_out[8];
#pragma GCC pop

volatile uint32_t g_start_ctr = 0;
volatile uint32_t g_stop_ctr = 0;

volatile uint64_t buffer_a[BUFFER_SIZE / sizeof(uint64_t)];
volatile uint64_t buffer_b[BUFFER_SIZE / sizeof(uint64_t)];

int main(void) {
    mutex_lock(g_start_lock);
    g_start_ctr++;
    mutex_unlock(g_start_lock);

    if (me() == 0) {
        perfcounter_config(COUNT_CYCLES, true);
    }

    while (g_start_ctr < 16) {}

    const volatile uint64_t* a = &buffer_a[THREAD_SIZE * me()];
    volatile uint64_t* b = &buffer_b[THREAD_SIZE * me()];

    for (size_t j = 0; j < 1000; ++j) {
#pragma unroll
        for (size_t i = 0; i < THREAD_SIZE; ++i) {
            b[i] = a[i];
        }
    }

    if (me() == 0) {
        while (g_stop_ctr < NR_TASKLETS) {}

        __ime_debug_out[0] = perfcounter_get();
    }
}
