#include <defs.h>
#include <stdint.h>
#include <assert.h>

volatile uint64_t a = 0x9ace1765afc17fa1;
volatile uint64_t b = 0x3b14d3aaf52bc131;

uint64_t add(volatile uint64_t a, volatile uint64_t b) {
    return a + b;
}

int main(void) {
    if (me() == 0) {
        __asm__ volatile("resume id, 0x1, false, 0x0");
    }

    assert(add(a, b) == 0x9ace1765afc17fa1 + 0x3b14d3aaf52bc131);

    if (me() == 0) {
        for (volatile int i = 0; i < 100000; ++i) {}
    } else {
        // __asm__ volatile("stop");
    }
}