.text
.globl __bootstrap

__bootstrap:
    lsl r22, id, 9
    add r22, r22, __ime_stack_start
    call r23, main

    jump zero, 0x1
