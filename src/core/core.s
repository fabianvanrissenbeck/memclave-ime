.text
.globl __bootstrap

__bootstrap:
    jump zero, core_on_boot
    jump zero, core_on_replace
    jump zero, core_on_signal
    jump zero, core_on_counter
    jump zero, core_on_chacha
    jump zero, poly_init
    jump zero, poly_feed_block
    jump zero, poly_finalize

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
    add r0, r0, 0x10000
    addc r1, r1, 0x0
    addc r2, r2, 0x0
    addc r3, r3, 0x0
    addc zero, zero, 0x0, nz, ime_sec_fault

    // r0 is treated in a special way. Multiple DPUs may use
    // the same encryption and decryption key, so each DPU must use
    // different IV values to not expose the XOR of the plaintext.
    // r0 has the following structure:
    // r0 = <16-bit counter> | <15-bit DPU ID> | <1 bit>
    //
    // The lowest bit is used, so that the guest can simply use IVs without this bit set.
    // This will prevent any accidental IV reuses.
    // The DPU ID ensures that each DPU has a unique counter.
    // The higher bits are normal counters.

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
