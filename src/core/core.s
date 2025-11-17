.text
.globl __bootstrap

__bootstrap:
    jump zero, core_on_boot
    jump zero, core_on_replace
    jump zero, core_on_signal
    jump zero, core_on_counter
    jump zero, core_on_chacha

core_on_boot:
    xor zero, id, 23, z, core_on_ctr_thread
    xor zero, id, 22, z, core_on_chacha_thread

    /* when the ci-switch resolves a fault it starts the faulting thread at pc=0. Handle this case */
    xor zero, lneg, r23, z, core_on_sigret

    xor zero, id, 0x0, z, core_cont_boot
    stop true, __ime_user_start

core_cont_boot:
    move r0, 0x3f00000 // 63 MiB - location of the messaging kernel
    move r1, 0x0       // no verification for now
    move r2, 0x0       // use system key
    move r3, 0x0       // don't wipe on boot

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

core_on_counter:
    xor zero, id, 0, nz, ime_sanity_fault
    move r1, __ime_threadio_start
    move r2, lneg
    sw r1, 60, r2

    boot zero, 23, nz, ime_sec_fault
    xor zero, id, 0, nz, ime_sec_fault

core_on_counter_loop:
    lw r2, r1, 60
    xor zero, r2, 0x0, nz, core_on_counter_loop

    lw r2, r1, 0x0
    sw r0, 0x0, r2
    lw r2, r1, 0x4
    sw r0, 0x4, r2
    lw r2, r1, 0x8
    sw r0, 0x8, r2
    lw r2, r1, 0xC
    sw r0, 0xC, r2

    jump r23

core_on_ctr_thread:
    add r0, r0, 0x1
    addc r1, r1, 0x0
    addc r2, r2, 0x0
    addc r3, r3, 0x0
    addc zero, zero, 0x0, nz, ime_sec_fault

    move r4, __ime_threadio_start

    sw r4, 0x0, r0
    sw r4, 0x4, r1
    sw r4, 0x8, r2
    sw r4, 0xC, r3
    sw r4, 60, 0x0

    stop t, __bootstrap

core_on_chacha_thread:
    jump ime_chacha_thread

core_on_chacha:
    jump ime_chacha_blk
