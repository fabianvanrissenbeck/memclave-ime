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

    move r12, r0
    move r13, r1
    move r14, r2
    move r15, r3

    move r0, 0x61707865
    move r1, 0x3320646e
    move r2, 0x79622d32
    move r3, 0x6b206574

    // dummy key - replace it
    move r4, 0x83828180
    move r5, 0x87868584
    move r6, 0x8b8a8988
    move r7, 0x8f8e8d8c
    move r8, 0x93929190
    move r9, 0x97969594
    move r10, 0x9b9a9998
    move r11, 0x9f9e9d9c

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

    lw r16, r22, 0
    add r12, r12, r16
    lw r16, r22, 4
    add r13, r13, r16
    lw r16, r22, 8
    add r14, r14, r16
    lw r16, r22, 12
    add r15, r15, r16

    add r0, r0, 0x61707865
    add r1, r1, 0x3320646e
    add r2, r2, 0x79622d32
    add r3, r3, 0x6b206574

    add r4, r4, 0x83828180
    add r5, r5, 0x87868584
    add r6, r6, 0x8b8a8988
    add r7, r7, 0x8f8e8d8c
    add r8, r8, 0x93929190
    add r9, r9, 0x97969594
    add r10, r10, 0x9b9a9998
    add r11, r11, 0x9f9e9d9c

    sw zero, chacha_output, r0
    sw zero, chacha_output + 4, r1
    sw zero, chacha_output + 8, r2
    sw zero, chacha_output + 12, r3
    sw zero, chacha_output + 16, r4
    sw zero, chacha_output + 20, r5
    sw zero, chacha_output + 24, r6
    sw zero, chacha_output + 28, r7
    sw zero, chacha_output + 32, r8
    sw zero, chacha_output + 36, r9
    sw zero, chacha_output + 40, r10
    sw zero, chacha_output + 44, r11
    sw zero, chacha_output + 48, r12
    sw zero, chacha_output + 52, r13
    sw zero, chacha_output + 56, r14
    sw zero, chacha_output + 60, r15

    jump r23

fault:
    fault 0x3

.data
.globl chacha_output
chacha_output:
    .fill 64