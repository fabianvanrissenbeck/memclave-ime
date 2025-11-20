#include <ime.h>
#include <defs.h>
#include <stddef.h>
#include <assert.h>
#include <perfcounter.h>

#include "../poly/poly.h"

#define SK_ADDR ((void __mram_ptr*) (63 << 20))
#define OUTPUT ((uint64_t __mram_ptr*) ((64 << 20) - 64))

enum {
    FIRST_INVOC = 0x30130bc26bfae030,
    SECOND_INVOC = 0xfc2ab8326bb5040a
};

__mram uint64_t is_first_invoc;
volatile uint32_t data[2048] = { 1 };

int main(void) {
    if (me() >= 24) {
        poly_init(NULL, NULL);
        poly_feed_block(NULL, NULL);
        poly_finalize(NULL, NULL);
    }

    if (is_first_invoc == SECOND_INVOC) {
        uint64_t tm = perfcounter_get();
        is_first_invoc = FIRST_INVOC;

        OUTPUT[0] = tm;
        asm("stop");
    } else {
        for (int i = 1; i < 2048; ++i) {
            assert(data[i] == 0);
        }

        perfcounter_config(COUNT_CYCLES, true);
        is_first_invoc = SECOND_INVOC;
    }

    return 0;
}