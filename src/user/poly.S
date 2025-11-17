#define POLY_ADD_REDUCE(t1, t2, t3, t4, t5, s1, s2, s3, s4, s5, n1, n2) \
    add t1, t1, s1; \
    addc t2, t2, s2; \
    addc t3, t3, s3; \
    addc t4, t4, s4; \
    addc t5, t5, s5; \
    \
    sub n1, t5, 0x3, gts; /* 1 if t5 > 0x3 and 0 otherwise */ \
    xor n1, n1, 0x1;  /* invert 0 if t5 > 0x3 and 1 otherwise */ \
    add n1, n1, -1;    /* UINT32_MAX if t5 > 0x3 and 0 otherwise */ \
    \
    and n2, n1, 0xFFFFFFFC; \
    and n1, n1, 0x5; \
    \
    add t1, t1, n1; \
    addc t2, t2, 0x0; \
    addc t3, t3, 0x0; \
    addc t4, t4, 0x0; \
    addc t5, t5, n2;

.text
.globl poly_masked_reduce
.globl poly_add_assign_4
.globl poly_add_reduce
.globl poly_mul_assign

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

    POLY_ADD_REDUCE(r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r7, r8)

    sw r0, 0x0, r2
    sw r0, 0x4, r3
    sw r0, 0x8, r4
    sw r0, 0xC, r5
    sw r0, 0x10, r6

    jump r23

poly_mul_assign:
    /*
        r0 - target address (ctx->a)
        r1 - source address (ctx->r)
        r2 - counter from 0 to 31 (inclusive) - used four separate times
        r3 - current 32-bits of key - reloaded four times
        [r4, r8] - product storage
        [r9, r13] - accumulator storage
        [r14, r15] - temporary storage
    */

    sw r22, 0x0, r14
    sw r22, 0x4, r15

    move r4, 0x0
    move r5, 0x0
    move r6, 0x0
    move r7, 0x0
    move r8, 0x0

    lw r9, r0, 0x0
    lw r10, r0, 0x4
    lw r11, r0, 0x8
    lw r12, r0, 0xC
    lw r13, r0, 0x10

    move r2, 32
    lw r3, r1, 0x0
poly_mul_assign_loop_0:
    and zero, r3, 0x1, z, poly_mul_assign_loop_0_double
    POLY_ADD_REDUCE(r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15)

poly_mul_assign_loop_0_double:
    POLY_ADD_REDUCE(r9, r10, r11, r12, r13, r9, r10, r11, r12, r13, r14, r15)

    lsr r3, r3, 1
    add r2, r2, -1, nz, poly_mul_assign_loop_0

    move r2, 32
    lw r3, r1, 0x4
poly_mul_assign_loop_1:
    and zero, r3, 0x1, z, poly_mul_assign_loop_1_double
    POLY_ADD_REDUCE(r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15)

poly_mul_assign_loop_1_double:
    POLY_ADD_REDUCE(r9, r10, r11, r12, r13, r9, r10, r11, r12, r13, r14, r15)

    lsr r3, r3, 1
    add r2, r2, -1, nz, poly_mul_assign_loop_1

    move r2, 32
    lw r3, r1, 0x8
poly_mul_assign_loop_2:
    and zero, r3, 0x1, z, poly_mul_assign_loop_2_double
    POLY_ADD_REDUCE(r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15)

poly_mul_assign_loop_2_double:
    POLY_ADD_REDUCE(r9, r10, r11, r12, r13, r9, r10, r11, r12, r13, r14, r15)

    lsr r3, r3, 1
    add r2, r2, -1, nz, poly_mul_assign_loop_2

    move r2, 32
    lw r3, r1, 0xC
poly_mul_assign_loop_3:
    and zero, r3, 0x1, z, poly_mul_assign_loop_3_double
    POLY_ADD_REDUCE(r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15)

poly_mul_assign_loop_3_double:
    POLY_ADD_REDUCE(r9, r10, r11, r12, r13, r9, r10, r11, r12, r13, r14, r15)

    lsr r3, r3, 1
    add r2, r2, -1, nz, poly_mul_assign_loop_3

    sw r0, 0x0, r4
    sw r0, 0x4, r5
    sw r0, 0x8, r6
    sw r0, 0xC, r7
    sw r0, 0x10, r8

    lw r14, r22, 0x0
    lw r15, r22, 0x4

    jump r23
