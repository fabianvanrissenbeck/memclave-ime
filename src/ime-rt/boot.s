.section .text.__bootstrap
.globl __bootstrap
.globl __ime_wait_for_host
.globl __ime_replace_sk
.globl __ime_get_counter

__bootstrap:
    sub zero, NR_TASKLETS-1, id, mi, stop_unused_tasklet

    lsl r22, id, 9 // 512 bytes stack per thread
    add r22, r22, __sys_stack_thread_0
    call r23, main

    call r23, __ime_stop_tasklet

    move r0, __ime_msg_sk
    move r1, 0x0
    move r2, 0x0
    move r3, __ime_persist_start

    jump zero, 0x1

// unused tasklets aren't counted und must not be stopped via __ime_stop_tasklet
stop_unused_tasklet:
    stop t, __bootstrap

__ime_replace_sk:
    move r3, __ime_persist_start
    jump 0x1

__ime_wait_for_host:
    jump 0x2

__ime_get_counter:
    jump 0x3