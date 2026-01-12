#include <ime.h>
#include <defs.h>
#include <stdint.h>

// include random stuff to increase .text size
#include <aead.h>
#include <buddy_alloc.h>

__mram volatile uint32_t g_stats[4];

void never_actually_called(void) {
    // increases .data size
    volatile static uint32_t data[2048] = { 1 };
    data[me()] = 123;

    // increases .text size
    ime_aead_enc(NULL, NULL, 0, NULL, NULL, NULL, NULL);
    buddy_alloc(123);
}

int main(void) {
    if (me() > 24) {
        never_actually_called();
    }

    // The TL (if in IME_REPORT_STATS mode) puts some stats
    // into the debug section im MRAM. Pull it out so that
    // returning to MSG does not clear this data.

    for (int i = 0; i < 4; i++) {
        // account for "imprecision" of time
        g_stats[i] = __ime_debug_out[i] << 4;
    }
}