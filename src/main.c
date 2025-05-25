#include "boot.h"

#include <defs.h>
#include <stdio.h>

static void waitcycles(uint32_t n) {
    for (volatile uint32_t i = 0; i < n; i++) {}
}

static void slave_main(void) {
    if ((me() & 1) == 0) {
        waitcycles(100000);
    }

    printf("[Thread %d] thrd_get_state() = %04x\n", me(), thrd_get_state());
}

int main(void) {
    if (me() == 0) {
        printf("thrd_boot_all() = %d\n", thrd_boot_all());

        waitcycles(10000);

        printf("thrd_get_state() = %04x\n", thrd_get_state());
    } else {
        slave_main();
    }
}