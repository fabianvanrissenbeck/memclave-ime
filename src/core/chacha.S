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

ime_chacha_blk:
    sw r22, 0, r0
    sw r22, 4, r1
    sw r22, 8, r2
    sw r22, 12, r3
    sw r22, 16, r4
    sw r22, 20, r5

    // r14-r16 are callee saved - this has caused issues
    sw r22, 24, r14
    sw r22, 28, r15
    sw r22, 32, r16

    move r12, r1
    move r13, r2
    move r14, r3
    move r15, r4

    xor zero, r0, 0x0, nz, chacha_load_key

    // dummy key - replace it
    move r4, 0x83828180
    move r5, 0x87868584
    move r6, 0x8b8a8988
    move r7, 0x8f8e8d8c
    move r8, 0x93929190
    move r9, 0x97969594
    move r10, 0x9b9a9998
    move r11, 0x9f9e9d9c

    jump chacha_key_loaded

chacha_load_key:
    lw r4, r0, 0x0
    lw r5, r0, 0x4
    lw r6, r0, 0x8
    lw r7, r0, 0xC
    lw r8, r0, 0x10
    lw r9, r0, 0x14
    lw r10, r0, 0x18
    lw r11, r0, 0x1C

chacha_key_loaded:

    move r0, 0x61707865
    move r1, 0x3320646e
    move r2, 0x79622d32
    move r3, 0x6b206574

    move r16, 10
chacha_loop:
    QUARTERROUND(r0, r4, r8, r12)
    QUARTERROUND(r1, r5, r9, r13)
    QUARTERROUND(r2, r6, r10, r14)
    QUARTERROUND(r3, r7, r11, r15)
    QUARTERROUND(r0, r5, r10, r15)
    QUARTERROUND(r1, r6, r11, r12)
    QUARTERROUND(r2, r7, r8, r13)
    QUARTERROUND(r3, r4, r9, r14)

    sub r16, r16, 1, nz, chacha_loop

    lw r16, r22, 4
    add r12, r12, r16
    lw r16, r22, 8
    add r13, r13, r16
    lw r16, r22, 12
    add r14, r14, r16
    lw r16, r22, 16
    add r15, r15, r16

    add r0, r0, 0x61707865
    add r1, r1, 0x3320646e
    add r2, r2, 0x79622d32
    add r3, r3, 0x6b206574

    lw r16, r22, 0x0
    xor zero, r16, 0x0, nz, chacha_add_key

    add r4, r4, 0x83828180
    add r5, r5, 0x87868584
    add r6, r6, 0x8b8a8988
    add r7, r7, 0x8f8e8d8c
    add r8, r8, 0x93929190
    add r9, r9, 0x97969594
    add r10, r10, 0x9b9a9998
    add r11, r11, 0x9f9e9d9c

    jump chacha_key_added

chacha_add_key:
    lw r16, r16, 0x0
    add r4, r4, r16
    lw r16, r22, 0x0
    lw r16, r16, 0x4
    add r5, r5, r16
    lw r16, r22, 0x0
    lw r16, r16, 0x8
    add r6, r6, r16
    lw r16, r22, 0x0
    lw r16, r16, 0xC
    add r7, r7, r16

    lw r16, r22, 0x0
    lw r16, r16, 0x10
    add r8, r8, r16
    lw r16, r22, 0x0
    lw r16, r16, 0x14
    add r9, r9, r16
    lw r16, r22, 0x0
    lw r16, r16, 0x18
    add r10, r10, r16
    lw r16, r22, 0x0
    lw r16, r16, 0x1C
    add r11, r11, r16

chacha_key_added:
    lw r16, r22, 20

    sw r16, 0, r0
    sw r16, 0 + 4, r1
    sw r16, 0 + 8, r2
    sw r16, 0 + 12, r3
    sw r16, 0 + 16, r4
    sw r16, 0 + 20, r5
    sw r16, 0 + 24, r6
    sw r16, 0 + 28, r7
    sw r16, 0 + 32, r8
    sw r16, 0 + 36, r9
    sw r16, 0 + 40, r10
    sw r16, 0 + 44, r11
    sw r16, 0 + 48, r12
    sw r16, 0 + 52, r13
    sw r16, 0 + 56, r14
    sw r16, 0 + 60, r15

    lw r14, r22, 24
    lw r15, r22, 28
    lw r16, r22, 32

    jump r23

fault:
    fault 0x3
