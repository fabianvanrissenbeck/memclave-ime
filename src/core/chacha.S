// expands to twelve instructions performing a quarter round
// on four registers as specified in rfc7539
#define QUARTERROUND(r0, r1, r2, r3) \
    add r0, r0, r1; \
    xor r3, r3, r0; \
    rol r3, r3, 16; \
                    \
    add r2, r2, r3; \
    xor r1, r1, r2; \
    rol r1, r1, 12; \
                    \
    add r0, r0, r1; \
    xor r3, r3, r0; \
    rol r3, r3, 8;  \
                    \
    add r2, r2, r3; \
    xor r1, r1, r2; \
    rol r1, r1, 7

.text
.globl ime_chacha_blk
.globl ime_chacha_thread

ime_chacha_blk:
    xor zero, r0, 0x0, nz, ime_chacha_user_key
    xor zero, id, 0x0, nz, ime_sanity_fault

ime_chacha_sys_key:
    move r6, __ime_threadio_start
    move r7, lneg
    sw r6, 60, r7

    sw r6, 0x00, r0
    sw r6, 0x04, r1
    sw r6, 0x08, r2
    sw r6, 0x0C, r3
    sw r6, 0x10, r4
    sw r6, 0x14, r5

    boot zero, 22, nz, ime_sec_fault
    call r0, ime_check_system

ime_chacha_wait_loop:
    lw r0, r6, 60
    xor zero, r0, 0x0, nz, ime_chacha_wait_loop

    jump r23

ime_chacha_thread:
    // immutable key registers - dummy: should be set by hypervisor/ci-switch
    move r16, 0x83828180
    move r17, 0x87868584
    move r18, 0x8b8a8988
    move r19, 0x8f8e8d8c
    move r20, 0x93929190
    move r21, 0x97969594
    move r22, 0x9b9a9998
    move r23, 0x9f9e9d9c

    move r0, __ime_threadio_start

    // back up the value at location NULL - we use it to store one value of the
    // fully computed state matrix to free a register

    lw r1, zero, 0x0
    sw r0, 0x18, r1

    lw r5, r0, 0x14
    lw r4, r0, 0x10
    lw r3, r0, 0x0C
    lw r2, r0, 0x08
    lw r1, r0, 0x04
    lw r0, r0, 0x00

    move r12, r1
    move r13, r2
    move r14, r3
    move r15, r4

    move r4, r16
    move r5, r17
    move r6, r18
    move r7, r19
    move r8, r20
    move r9, r21
    move r10, r22
    move r11, r23

chacha_block_entry:
    move r0, 0x61707865
    move r1, 0x3320646e
    move r2, 0x79622d32
    move r3, 0x6b206574

    // use run bits as loop counter because no other registers are free

    clr_run zero, 24, t, .+1
    clr_run zero, 25, t, .+1
    clr_run zero, 26, t, .+1
    clr_run zero, 27, t, .+1

chacha_loop:
    QUARTERROUND(r0, r4, r8, r12)
    QUARTERROUND(r1, r5, r9, r13)
    QUARTERROUND(r2, r6, r10, r14)
    QUARTERROUND(r3, r7, r11, r15)
    QUARTERROUND(r0, r5, r10, r15)
    QUARTERROUND(r1, r6, r11, r12)
    QUARTERROUND(r2, r7, r8, r13)
    QUARTERROUND(r3, r4, r9, r14)

    clr_run zero, 27, nz, chacha_loop_lt_8

    boot zero, 24, z, chacha_loop
    clr_run zero, 24, t, .+1
    boot zero, 25, z, chacha_loop
    clr_run zero, 25, t, .+1
    boot zero, 26, z, chacha_loop
    clr_run zero, 26, t, .+1
    boot zero, 27, t, chacha_loop

chacha_loop_lt_8:
    boot zero, 27
    boot zero, 24, z, chacha_loop

    // sub r16, r16, 1, nz, chacha_loop

    // add nothing up my sleeve numbers

    add r0, r0, 0x61707865
    add r1, r1, 0x3320646e
    add r2, r2, 0x79622d32
    add r3, r3, 0x6b206574

    xor zero, id, 22, nz, ime_chacha_user_exit

    // write the first state matrix cell to the zero page
    // then use r0 to store the three next cells properly
    // restore zero page state afterwards

    sw zero, 0x0, r0
    move r0, __ime_threadio_start
    lw r0, r0, 0x14

    sw r0, 0x4, r1
    sw r0, 0x8, r2
    sw r0, 0xC, r3

    lw r1, zero, 0x0
    sw r0, 0x0, r1

    move r0, __ime_threadio_start
    lw r1, r0, 0x18
    sw zero, 0x0, r1

    // add counter and iv to state matrix

    lw r1, r0, 0x4
    add r12, r12, r1

    lw r1, r0, 0x8
    add r13, r13, r1

    lw r1, r0, 0xC
    add r14, r14, r1

    lw r1, r0, 0x10
    add r15, r15, r1

    add r4, r4, r16
    add r5, r5, r17
    add r6, r6, r18
    add r7, r7, r19
    add r8, r8, r20
    add r9, r9, r21
    add r10, r10, r22
    add r11, r11, r23

    lw r1, r0, 20

    sw r1, 0 + 16, r4
    sw r1, 0 + 20, r5
    sw r1, 0 + 24, r6
    sw r1, 0 + 28, r7
    sw r1, 0 + 32, r8
    sw r1, 0 + 36, r9
    sw r1, 0 + 40, r10
    sw r1, 0 + 44, r11
    sw r1, 0 + 48, r12
    sw r1, 0 + 52, r13
    sw r1, 0 + 56, r14
    sw r1, 0 + 60, r15

ime_chacha_thread_end:
    sw r0, 60, 0x0
    stop t, __bootstrap

ime_chacha_user_key:
    sw r22, 0x00, r0
    sw r22, 0x04, r1
    sw r22, 0x08, r2
    sw r22, 0x0C, r3
    sw r22, 0x10, r4
    sw r22, 0x14, r5

    sw r22, 0x18, r14
    sw r22, 0x1C, r15
    sw r22, 0x20, r16

    move r12, r1
    move r13, r2
    move r14, r3
    move r15, r4

    lw r4, r0, 0x0
    lw r5, r0, 0x4
    lw r6, r0, 0x8
    lw r7, r0, 0xC
    lw r8, r0, 0x10
    lw r9, r0, 0x14
    lw r10, r0, 0x18
    lw r11, r0, 0x1C

    jump chacha_block_entry

ime_chacha_user_exit:

    lw r16, r22, 0x4
    add r12, r12, r16
    lw r16, r22, 0x8
    add r13, r13, r16
    lw r16, r22, 0xC
    add r14, r14, r16
    lw r16, r22, 0x10
    add r15, r15, r16

    lw r16, r22, 0x14

    sw r16, 0x30, r12
    sw r16, 0x34, r13
    sw r16, 0x38, r14
    sw r16, 0x3C, r15

    lw r14, r22, 0x0

    lw r15, r14, 0x0
    add r4, r4, r15
    lw r15, r14, 0x4
    add r5, r5, r15
    lw r15, r14, 0x8
    add r6, r6, r15
    lw r15, r14, 0xC
    add r7, r7, r15

    lw r15, r14, 0x10
    add r8, r8, r15
    lw r15, r14, 0x14
    add r9, r9, r15
    lw r15, r14, 0x18
    add r10, r10, r15
    lw r15, r14, 0x1C
    add r11, r11, r15

    sw r16, 0x00, r0
    sw r16, 0x04, r1
    sw r16, 0x08, r2
    sw r16, 0x0C, r3
    sw r16, 0x10, r4
    sw r16, 0x14, r5
    sw r16, 0x18, r6
    sw r16, 0x1C, r7
    sw r16, 0x20, r8
    sw r16, 0x24, r9
    sw r16, 0x28, r10
    sw r16, 0x2C, r11

    lw r14, r22, 0x18
    lw r15, r22, 0x1C
    lw r16, r22, 0x20

    jump r23
