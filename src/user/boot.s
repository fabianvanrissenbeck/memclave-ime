.text
.globl __bootstrap

__bootstrap:
    sub zero, NR_TASKLETS-1, id, mi, stop_unused_tasklet

    lsl r22, id, 9
    add r22, r22, __sys_stack_thread_0
    call r23, main

    move r0, 0x0
    move r1, 0x0
    move r2, 0x0
    move r3, 0x1

    jump zero, 0x1

stop_unused_tasklet:
    stop t, __bootstrap
