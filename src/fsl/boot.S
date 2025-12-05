.text
.globl __bootstrap
.globl install_keys

__bootstrap:
    xor zero, id, 22, z, on_thread_22
    xor zero, id, 23, z, on_thread_23
    move r22, __sys_stack_thread_0
    call r23, main
    stop

install_keys:
    // make sure this is called from thread 0
    xor zero, id, 0, z, .+2
    fault 0x3

    boot id, 22
    boot id, 23

    move r0, 0x20000

    // wait a few instructions on thread 22 and 23
    // i was to lazy for explizit synchronization
    add r0, r0, -1, nz, .

    // make sure thread 22 terminated
    clr_run zero, 22, z, .+2
    fault 0x3

    // make sure thread 23 terminated
    clr_run zero, 23, z, .+2
    fault 0x3

    jump r23

on_thread_22:
    lw r16, zero, sys_key
    lw r17, zero, sys_key + 4
    lw r18, zero, sys_key + 8
    lw r19, zero, sys_key + 12
    lw r20, zero, sys_key + 16
    lw r21, zero, sys_key + 20
    lw r22, zero, sys_key + 24
    lw r23, zero, sys_key + 28

    stop true, __bootstrap

on_thread_23:
    lw r0, zero, dpu_index
    move r1, 0x0
    move r2, 0x0
    move r3, 0x0

    stop true, __bootstrap