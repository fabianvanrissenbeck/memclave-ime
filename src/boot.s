.text
.globl __bootstrap
.globl boot_slave_threads
.globl kill_slave_threads

__bootstrap:
    sub zero, 15, id, mi, fault_to_many_threads
    xor zero, id, 15, z, stop_to_entry
    clr_run id, 1, false, 0x0
    boot id, 1, false, 0x0
    xor zero, id, 0, z, __entry_master

stop_to_entry:
    sw id4, thread_state_buf, 0
    stop t, __entry_slave


__entry_master:
    move r22, __sys_stack_thread_0
    call r23, main
    stop t, __bootstrap

__entry_slave:
    lsl r22, id, 9
    add r22, r22, __sys_stack_thread_0
    sw id4, thread_state_buf, 1
    call r23, slave_thread_main
    sw id4, thread_state_buf, 0
    stop t, __entry_slave

boot_slave_threads:
    move r0, 0x1

bst_loop:
    resume r0, 0, false, 0x0
    add r0, r0, 0x1
    xor zero, r0, 16, nz, bst_loop

    move r0, 0x0
    jump r23

kill_slave_threads:

    clr_run id, 1, false, 0x0
    clr_run id, 2, false, 0x0
    clr_run id, 3, false, 0x0
    clr_run id, 4, false, 0x0
    clr_run id, 5, false, 0x0
    clr_run id, 6, false, 0x0
    clr_run id, 7, false, 0x0
    clr_run id, 8, false, 0x0
    clr_run id, 9, false, 0x0
    clr_run id, 10, false, 0x0
    clr_run id, 11, false, 0x0
    clr_run id, 12, false, 0x0
    clr_run id, 13, false, 0x0
    clr_run id, 14, false, 0x0
    clr_run id, 15, false, 0x0

    boot id, 1, false, 0x0
    boot id, 2, false, 0x0
    boot id, 3, false, 0x0
    boot id, 4, false, 0x0
    boot id, 5, false, 0x0
    boot id, 6, false, 0x0
    boot id, 7, false, 0x0
    boot id, 8, false, 0x0
    boot id, 9, false, 0x0
    boot id, 10, false, 0x0
    boot id, 11, false, 0x0
    boot id, 12, false, 0x0
    boot id, 13, false, 0x0
    boot id, 14, false, 0x0
    boot id, 15, false, 0x0

    jump r23


fault_to_many_threads:
    fault 0x3