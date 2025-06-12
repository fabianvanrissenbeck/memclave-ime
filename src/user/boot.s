.text
.globl __bootstrap

__bootstrap:
    lsl r22, id, 9
    add r22, r22, __ime_stack_start
    call r23, main

    move r0, 0x0
    move r1, 0x0
    move r2, 0x0
    move r3, 0x1

    jump zero, 0x1
