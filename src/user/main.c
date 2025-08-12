#include <ime.h>
#include <mram.h>

extern uint64_t __mram_noinit __ime_debug_out[8];

int main(void) {
    uint32_t ctr_a[16];

    __ime_get_counter(&ctr_a[0]);
    __ime_get_counter(&ctr_a[4]);
    __ime_get_counter(&ctr_a[8]);
    __ime_get_counter(&ctr_a[12]);

    uint32_t __mram_ptr* out = (uint32_t __mram_ptr*) &__ime_debug_out[0];

    for (int i = 0; i < 16; ++i) {
        out[i] = ctr_a[i];
    }

    asm("stop");
    return 0;
}