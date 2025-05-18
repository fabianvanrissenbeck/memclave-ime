.text
.globl boot_thread_num
.globl clear_run_bits
.globl __bootstrap

__bootstrap:
    lsl r22, id8, 5
    add r22, r22, __sys_stack_thread_0
    call r23, main
    stop t, 0x0

boot_thread_num:
    and r0, r0, 0xFF
    move r1, 0x0
    boot r0, 0x0, z, is_zero
    move r1, 0x1
is_zero:
    move r0, r1
    jump r23

clear_run_bits:
    move r1, 0xf
crb_loop_a:
    clr_run r1, 0x0, false, 0x0
    sub r1, r1, 0x1, nz, crb_loop_a

    move r1, 0xf
    move r0, 0x0
crb_loop:
    clr_run r1, 0x0, z, crb_loop_z
    or r0, r0, 0x1
crb_loop_z:
    lsl r0, r0, 1
    sub r1, r1, 0x1, nz, crb_loop

    or r0, r0, 0x1
    jump r23