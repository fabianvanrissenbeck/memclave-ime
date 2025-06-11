.text
.globl ime_load_wram

ime_load_wram:
    /* load header of subkernel */
    move r0, 0
    ldma r0, r21, 7

    /* sanity check for header validity */
    lw r1, r0, 0x0
    xor r1, r1, 0xA5A5A5A5
    xor zero, r1, 0, nz, sanity_fault

    /* calculate offset to WRAM region in MRAM copy */
    lw r1, r0, 40
    lsl r1, r1, 11
    add r1, r1, 64
    add r1, r1, r21

    /* load amount of loads required and target WRAM address */
    lw r2, r0, 44
    move r0, 0

    xor zero, r2, 0, z, clw_end
clw_loop:
    ldma r0, r1, 255
    add r0, r0, 2048
    add r1, r1, 2048
    sub r2, r2, 1, nz, clw_loop
clw_end:

    jump r23

sanity_fault:
    fault 0x3