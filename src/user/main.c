#include <defs.h>
#include <stdint.h>
#include <assert.h>

volatile uint64_t a = 0x9ace1765afc17fa1;
volatile uint64_t b = 0x3b14d3aaf52bc131;

uint64_t add(volatile uint64_t a, volatile uint64_t b) {
    return a + b;
}

int main(void) {
    // assert(add(a, b) == 0x9ace1765afc17fa1 + 0x3b14d3aaf52bc131);

    volatile uint64_t __mram_ptr* test_ptr = (volatile uint64_t __mram_ptr*) ((64 << 20) - 64);
    *test_ptr = 0xF0F0F0F0;
    assert(*test_ptr == 0xF0F0F0F0);

    asm("stop");
}