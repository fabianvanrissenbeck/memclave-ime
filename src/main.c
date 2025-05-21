#include <defs.h>
#include <stdio.h>
#include <stdint.h>

volatile uint64_t count[16] = { 0 };
volatile uint32_t thread_state_buf[16] = { 0 };

/**
 * assumes that all slave threads are stopped at __entry_slave
 */
extern void boot_slave_threads(void);

extern void kill_slave_threads(void);

int slave_thread_main(void) {
    while (1) {
        count[me()] += 1;
    }

    return 0;
}

uint32_t get_thread_state(void) {
    uint32_t state = 0;

    for (int i = 0; i < 16; i++) {
        if (thread_state_buf[i]) {
            state |= 1 << i;
        }
    }

    return state;
}

void waitcycles(unsigned limit) {
    for (volatile unsigned i = 0; i < limit; i++) {}
}

uint64_t compute_sum(void) {
    uint64_t sum = 0;

    for (int i = 0; i < 16; i++) {
        sum += count[i];
    }

    return sum;
}

int main(void) {
    waitcycles(1000);
    printf("before boot: get_thread_state() = %04x\n", get_thread_state());

    boot_slave_threads();

    waitcycles(1000000);
    printf("before killing: get_thread_state() = %04x\n", get_thread_state());
    printf("sum = %lu\n", compute_sum());

    kill_slave_threads();

    waitcycles(1000000);

    printf("after killing: get_thread_state() = %04x\n", get_thread_state());
    printf("sum = %lu\n", compute_sum());

    waitcycles(100000);
    return 0;
}