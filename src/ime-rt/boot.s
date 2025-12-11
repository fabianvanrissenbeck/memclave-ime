.section .text.__bootstrap
.globl __bootstrap
.globl __ime_wait_for_host
.globl __ime_replace_sk
.globl __ime_get_counter
.globl __ime_chacha_blk
.globl poly_init
.globl poly_feed_block
.globl poly_finalize

__bootstrap:
    sub zero, NR_TASKLETS-1, id, mi, stop_unused_tasklet

    lsl r22, id, 10 // 512 bytes stack per thread
    add r22, r22, __sys_stack_thread_0
    call r23, main

    call r23, __ime_stop_tasklet
    // here all threads except for 0 have stopped or will at least no longer use atomic memory
    // therefore I can safely reset it
    call r23, reset_atomic

    move r0, g_load_prop
    move r1, 12

wipe_key_loop:
    sw r0, 0x0, 0x0
    add r0, r0, 0x4
    add r1, r1, -0x1, nz, wipe_key_loop

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

__ime_chacha_blk:
    jump 0x4

poly_init:
    jump 0x5

poly_feed_block:
    jump 0x6

poly_finalize:
    jump 0x7

reset_atomic:
    move r0, 255
reset_atomic_loop:
    release r0, 0x0, nz, .+1
    add r0, r0, -1, pl, reset_atomic_loop

    jump r23