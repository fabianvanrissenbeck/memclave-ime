#include "global.h"
#include "core.h"

#include <mram.h>

__attribute__((aligned(8)))
__attribute__((section(".data.persistent")))
uint32_t buffer[16 + 2] = {
    [0] = 0xA5A5A5A5,
    [10] = 1,
    [11] = 0,
#if 1
    [16] = 0x20300000,
    [17] = 0x00007e63
#else
    [16] = 0x20000000,
    [17] = 0x00007ef3,
#endif
};

int main(void) {
    extern __mram uint64_t __ime_swap_start[];

    mram_write(buffer, __ime_swap_start, sizeof(buffer));
    ime_sk_addr = &__ime_swap_start[0];

    core_replace_sk();
    return 0;
}