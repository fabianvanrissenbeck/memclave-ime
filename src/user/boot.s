.text
.globl __bootstrap

__bootstrap:
    move r22, __ime_stack_start
    call r23, main
    stop /* TODO: Change, but right now no follow up subkernel concept is implemented */
