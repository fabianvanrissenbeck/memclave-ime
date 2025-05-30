.text
.globl core_enter_system
.globl core_leave_system
.globl core_load_sub_kern

core_enter_system:
    move r20, __ime_user_start
    /* ldmai expects elf style addresses (so instruction index * 8) for some reason */
    lsl r20, r20, 3
    lw r21, zero, ime_sk_addr
    move r22, __ime_stack_start

    xor zero, id, 0, nz, core_sec_fault
    jump core_check_threads

core_leave_system:
    move r20, 4096
    move r21, 64 * 1024 * 1024
    jump __ime_user_start

core_sec_fault:
    move r20, 4096
    move r21, 64 * 1024 * 1024
    fault 0x101010

core_load_sub_kern:
    /* load header of subkernel */
    ldma r22, r21, 7

    /* add offset to IRAM section */
    add r21, r21, 64

    /* load the size of the sub kernel (should be between 1 and 15 (inclusive)) */
    lw r0, r22, 40

    /* fault if r0 is either 0 or larger than 15 */
    xor zero, r0, 0, z, core_sec_fault
    and r1, r0, 0xFFFFFFF0
    xor zero, r1, 0, nz, core_sec_fault

    /* convert r0 into an offset into the jump table: r0 = (15 - r0) * 4 */
    sub r0, 15, r0
    lsl r0, r0, 2

    /* branch depending on r0 */
    jump r0, cls_table_15


cls_table_15:
    ldmai r20, r21, 255
    add r20, r20, 2048
    add r21, r21, 2048
    nop

cls_table_14:
    ldmai r20, r21, 255
    add r20, r20, 2048
    add r21, r21, 2048
    nop

cls_table_13:
    ldmai r20, r21, 255
    add r20, r20, 2048
    add r21, r21, 2048
    nop

cls_table_12:
    ldmai r20, r21, 255
    add r20, r20, 2048
    add r21, r21, 2048
    nop

cls_table_11:
    ldmai r20, r21, 255
    add r20, r20, 2048
    add r21, r21, 2048
    nop

cls_table_10:
    ldmai r20, r21, 255
    add r20, r20, 2048
    add r21, r21, 2048
    nop

cls_table_9:
    ldmai r20, r21, 255
    add r20, r20, 2048
    add r21, r21, 2048
    nop

cls_table_8:
    ldmai r20, r21, 255
    add r20, r20, 2048
    add r21, r21, 2048
    nop

cls_table_7:
    ldmai r20, r21, 255
    add r20, r20, 2048
    add r21, r21, 2048
    nop

cls_table_6:
    ldmai r20, r21, 255
    add r20, r20, 2048
    add r21, r21, 2048
    nop

cls_table_5:
    ldmai r20, r21, 255
    add r20, r20, 2048
    add r21, r21, 2048
    nop

cls_table_4:
    ldmai r20, r21, 255
    add r20, r20, 2048
    add r21, r21, 2048
    nop

cls_table_3:
    ldmai r20, r21, 255
    add r20, r20, 2048
    add r21, r21, 2048
    nop

cls_table_2:
    ldmai r20, r21, 255
    add r20, r20, 2048
    add r21, r21, 2048
    nop

cls_table_1:
    ldmai r20, r21, 255
    add r20, r20, 2048
    add r21, r21, 2048
    nop

    jump core_unlock_mram