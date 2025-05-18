#include <defs.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief start a specific DPU thread
 * @param tid number of the thread
 * @return 0 if the thread was not running, 1 otherwise.
 */
extern bool boot_thread_num(uint8_t tid);

/**
 * clear the run bits of all threads but 0
 * @return state of the run bits after the first and before the second clear step
 */
extern uint16_t clear_run_bits(void);

int main(void) {
    if (me() == 0) {
        for (int i = 1; i < 16; ++i) {
            boot_thread_num(i);
        }

        for (volatile int i = 0; i < 1000; ++i) {}

        uint16_t bits = clear_run_bits();
        printf("RunBits: 0x%04x\n", bits);
    } else {
        while (1) {}
    }

    return 0;
}