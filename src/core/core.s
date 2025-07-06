.text
.globl __bootstrap

__bootstrap:
    jump zero, core_on_boot
    jump zero, core_on_replace
    jump zero, core_on_signal

core_on_boot:
    /* when the ci-switch resolves a fault it starts the faulting thread at pc=0. Handle this case */
    xor zero, lneg, r23, z, core_on_sigret

    xor zero, id, 0x0, z, core_cont_boot
    stop true, __ime_user_start

core_cont_boot:
    move r0, 0x3f00000 // 63 MiB - location of the messaging kernel
    move r1, 0x0       // no verification for now
    move r2, 0x0       // use system key

    jump zero, ime_replace_sk

/* used as a trampoline so that user code can always do jump 1 to call ime_replace_sk */
core_on_replace:
    jump zero, ime_replace_sk

core_on_signal:
    add r22, r22, 0x8
    sw r22, -0x8, r23
    move r23, lneg
    fault 0x101010

core_on_sigret:
    lw r23, r22, -0x8
    add r22, r22, -0x8
    jump r23
