.text
.globl ime_load_iram

ime_load_iram:
    /* load size of sub kernel without destroying WRAM (loaded before) */
    lw r0, zero, __ime_wram_start
    lw r1, zero, __ime_wram_start + 4

    /* add offset to size field */
    add r21, r21, 40

    /* load size and add offset to IRAM section (40 + 24 == 64) */
    ldma zero, r21, 0
    lw r2, zero, 0
    add r21, r21, 24

    /* restore WRAM content */
    sw zero, __ime_wram_start, r0
    sw zero, __ime_wram_start + 4, r1

    /* calculate return value */
    add r0, r21, -64

    /* fault if size is either 0 or larger than 15 */
    xor zero, r2, 0, z, ime_sec_fault
    and r1, r2, 0xFFFFFFF0
    xor zero, r1, 0, nz, ime_sec_fault

    /* convert r2 into an offset into the jump table: r0 = (15 - r0) * 4 */
    sub r2, 15, r2
    lsl r2, r2, 2

    /* branch depending on r2 */
    jump r2, cls_table_15

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
    nop
    nop
    nop

    jump r23
