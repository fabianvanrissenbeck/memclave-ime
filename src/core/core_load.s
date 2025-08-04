.text
.globl ime_replace_sk

ime_replace_sk:
    move r20, __ime_user_start
    /* ldmai expects elf style addresses (so instruction index * 8) for some reason */
    lsl r20, r20, 3

    move r17, r3
    move r18, r1
    move r19, r2
    move r21, r0

    call r23, ime_check_threads
    call r23, ime_lock_memory
    call r23, ime_wipe_user

    /*
     * ime_decrypt_sk and ime_scan_sk are implemented in c and strictly require a correctly setup
     * stack. That is why r22 is set to zero. user provided subkernels that which to persist contents
     * of their WRAM while crossing to another subkernel must make sure that the first few sections
     * of WRAM are not used by them for anything persistent.
     */
    move r0, r21
    move r1, r18
    move r2, r19
    move r22, __sys_stack_thread_0
    call r23, ime_decrypt_sk
    move r0, r21
    move r22, __sys_stack_thread_0
    call r23, ime_scan_sk

    call r23, ime_load_wram
    call r23, ime_load_iram

    call r23, ime_unlock_memory

    move r0, 0x0
    move r1, 0x0
    move r2, 0x0
    move r3, 0x0

    move r4, 0x0
    move r5, 0x0
    move r6, 0x0
    move r7, 0x0

    move r8, 0x0
    move r9, 0x0
    move r10, 0x0
    move r11, 0x0

    move r12, 0x0
    move r13, 0x0
    move r14, 0x0
    move r16, 0x0

    move r17, 0x0
    move r18, 0x0
    move r19, 0x0

    move r20, 4096 * 8
    move r21, 64 * 1024 * 1024

    move r0, NR_TASKLETS
resume_loop:
    sub r0, r0, 1, z, resume_end
    resume r0, 0
    jump resume_loop
resume_end:

    move r22, 0x0
    move r23, 0x0

    jump zero, __ime_user_start