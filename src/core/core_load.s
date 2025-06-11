.text
.globl ime_replace_sk

ime_replace_sk:
    move r20, __ime_user_start
    /* ldmai expects elf style addresses (so instruction index * 8) for some reason */
    lsl r20, r20, 3

    move r18, r1
    move r19, r2
    move r21, r0

    call r23, ime_check_threads
    call r23, ime_lock_memory

    move r0, r21
    move r1, r18
    move r2, r19
    call r23, ime_decrypt_sk
    move r0, r21
    call r23, ime_scan_sk
    call r23, ime_wipe_user

    call r23, ime_load_wram
    call r23, ime_load_iram

    call r23, ime_unlock_memory

    move r18, 0x0
    move r19, 0x0
    move r20, 4096 * 8
    move r21, 64 * 1024 * 1024

    jump zero, __ime_user_start