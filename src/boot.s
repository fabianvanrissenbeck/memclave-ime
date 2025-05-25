.text
.globl __bootstrap
.globl __thread_stop
.globl thrd_get_state
.globl thrd_boot_all

__bootstrap:
    xor zero, id, 0, z, continue_bootstrap
    stop t, __thread_stop

continue_bootstrap:
    move r0, 0x1

initial_boot_loop:
    boot r0, 0x0
    add r0, r0, 0x1
    xor zero, r0, NR_TASKLETS, nz, initial_boot_loop

    move r22, __sys_stack_thread_0
    call r23, main
    stop t, __bootstrap


__thread_stop:
    lsl r22, id, 9
    add r22, r22, __sys_stack_thread_0
    call r23, main
    stop t, __thread_stop


thrd_boot_all:
    move r0, lneg
    xor zero, id, 0, nz, thrd_boot_all_exit
    move r1, 0x1

thrd_boot_all_loop:
    resume r1, 0, false, 0x0
    add r1, r1, 0x1
    xor zero, r1, NR_TASKLETS, nz, thrd_boot_all_loop

    move r0, 0x0

thrd_boot_all_exit:
    jump r23


thrd_get_state:
    move r0, zero
    xor zero, id, 0, nz, thrd_get_state_exit

    move r0, NR_TASKLETS - 1

thrd_get_state_loop_1:
    boot r0, 0, false, 0x0
    sub r0, r0, 1, nz, thrd_get_state_loop_1

    move r0, 0x10000

thrd_get_state_loop_2:
    sub r0, r0, 1, nz, thrd_get_state_loop_2

    move r0, NR_TASKLETS - 1
    move r1, 0x0

thrd_get_state_loop_3:
    boot r0, 0, z, not_running
    or r1, r1, 0x1
not_running:
    lsl r1, r1, 1
    sub r0, r0, 1, nz, thrd_get_state_loop_3

    move r0, 0x10000

thrd_get_state_loop_4:
    sub r0, r0, 1, nz, thrd_get_state_loop_4

    or r0, r1, 1

thrd_get_state_exit:
    jump r23
