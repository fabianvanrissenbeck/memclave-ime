#include "ime.h"

#include <defs.h>
#include <mutex.h>

MUTEX_INIT(__ime_tasklet_lock);
static volatile uint32_t __ime_tasklet_count = NR_TASKLETS;

__attribute__((section(".data.persist.last"))) ime_load_params g_load_prop = { 0 };

void __ime_stop_tasklet(void) {
    if (me() == 0) {
        uint32_t count = NR_TASKLETS;

        while (count != 1) {
            mutex_lock(__ime_tasklet_lock);
            count = __ime_tasklet_count;
            mutex_unlock(__ime_tasklet_lock);

            for (volatile int i = 0; i < 1000; ++i) {}
        }
    } else {
        mutex_lock(__ime_tasklet_lock);
        __ime_tasklet_count -= 1;
        mutex_unlock(__ime_tasklet_lock);

        asm("stop t, __bootstrap");
    }
}
