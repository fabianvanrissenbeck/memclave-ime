.text
.globl poly_masked_reduce
.globl poly_add_assign_4
.globl poly_add_assign_5
.globl poly_add_reduce

poly_masked_reduce:
    lw r2, r0, 0x0
    lw r3, r0, 0x4
    lw r4, r0, 0x8
    lw r5, r0, 0xC
    lw r6, r0, 0x10

    and r7, r1, 0x5
    and r8, r1, 0x0
    and r9, r1, 0xFFFFFFFC

    add r2, r2, r7
    addc r3, r3, r8
    addc r4, r4, r8
    addc r5, r5, r8
    addc r6, r6, r9

    sw r0, 0x0, r2
    sw r0, 0x4, r3
    sw r0, 0x8, r4
    sw r0, 0xC, r5
    sw r0, 0x10, r6

    jump r23

// extern void poly_add_assign_5(uint32_t target[5], const uint32_t src[5]);

poly_add_assign_4:
    lw r2, r0, 0x0
    lw r3, r0, 0x4
    lw r4, r0, 0x8
    lw r5, r0, 0xC

    lw r7, r1, 0x0
    lw r8, r1, 0x4
    lw r9, r1, 0x8
    lw r10, r1, 0xC

    add r2, r2, r7
    addc r3, r3, r8
    addc r4, r4, r9
    addc r5, r5, r10

    sw r0, 0x0, r2
    sw r0, 0x4, r3
    sw r0, 0x8, r4
    sw r0, 0xC, r5

    jump r23

poly_add_assign_5:
    lw r2, r0, 0x0
    lw r3, r0, 0x4
    lw r4, r0, 0x8
    lw r5, r0, 0xC
    lw r6, r0, 0x10

    lw r7, r1, 0x0
    lw r8, r1, 0x4
    lw r9, r1, 0x8
    lw r10, r1, 0xC
    lw r11, r1, 0x10

    add r2, r2, r7
    addc r3, r3, r8
    addc r4, r4, r9
    addc r5, r5, r10
    addc r6, r6, r11

    sw r0, 0x0, r2
    sw r0, 0x4, r3
    sw r0, 0x8, r4
    sw r0, 0xC, r5
    sw r0, 0x10, r6

    jump r23

poly_add_reduce:
    lw r2, r0, 0x0
    lw r3, r0, 0x4
    lw r4, r0, 0x8
    lw r5, r0, 0xC
    lw r6, r0, 0x10

    lw r7, r1, 0x0
    lw r8, r1, 0x4
    lw r9, r1, 0x8
    lw r10, r1, 0xC
    lw r11, r1, 0x10

    add r2, r2, r7
    addc r3, r3, r8
    addc r4, r4, r9
    addc r5, r5, r10
    addc r6, r6, r11

    sub r1, r6, 0x3, gts // 1 if r6 > 0x3 and 0 otherwise
    xor r1, r1, 0x1  // invert 0 if r6 > 0x3 and 1 otherwise
    add r1, r1, -1    // UINT32_MAX if r6 > 0x3 and 0 otherwise

    and r7, r1, 0x5
    and r8, r1, 0x0
    and r9, r1, 0xFFFFFFFC

    add r2, r2, r7
    addc r3, r3, r8
    addc r4, r4, r8
    addc r5, r5, r8
    addc r6, r6, r9

    sw r0, 0x0, r2
    sw r0, 0x4, r3
    sw r0, 0x8, r4
    sw r0, 0xC, r5
    sw r0, 0x10, r6

    jump r23