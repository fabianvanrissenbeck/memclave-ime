#include <defs.h>
#include <stdint.h>
#include <assert.h>
#include <barrier.h>

// Note: I used a barrier with 17 entries to cause a deadlock. This causes dpurun to terminate
// because all threads are suspended. Barrier_wait actually calls stop and resume so that
// blocked threads don't spin on the acquire instruction. This also means that the current instruction
// scanning mechanism would not allow barriers (as implemented by UPMEM). The barrier with 16 entries
// works as expected.
BARRIER_INIT(some_test_barrier, 16)

volatile uint64_t a = 0x9ace1765afc17fa1;
volatile uint64_t b = 0x3b14d3aaf52bc131;

uint64_t add(volatile uint64_t a, volatile uint64_t b) {
    return a + b;
}

int main(void) {
    assert(add(a, b) == 0x9ace1765afc17fa1 + 0x3b14d3aaf52bc131);
    barrier_wait(&some_test_barrier);

    uint64_t __mram_ptr* debug = (uint64_t __mram_ptr*)((64 << 20) - 64);

    if (me() < 8) {
        debug[me()] = me();
    }

    asm("stop");
}
