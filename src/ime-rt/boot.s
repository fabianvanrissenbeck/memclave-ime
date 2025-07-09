.text
.globl __bootstrap
.globl __ime_wait_for_host
.globl __ime_replace_sk

__bootstrap:
    sub zero, NR_TASKLETS-1, id, mi, stop_unused_tasklet

    lsl r22, id, 9 // 512 bytes stack per thread
    add r22, r22, __sys_stack_thread_0
    call r23, main

    move r0, 0x3f00000
    move r1, 0x0
    move r2, 0x0
    move r3, 0x1

    jump zero, 0x1

stop_unused_tasklet:
    stop t, __bootstrap

__ime_replace_sk:
    jump 0x1

__ime_wait_for_host:
    jump 0x2