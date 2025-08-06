// Simple tool that writes to the tid to first NR_TASKLETS * 8 bytes in MRAM

#include <defs.h>
#include <mram.h>
#include <stddef.h>

int main(void) {
    uint64_t __mram_ptr* ptr = NULL;
    uint64_t tid = me();

    mram_write(&tid, &ptr[tid], sizeof(tid));
    return 0;
}
