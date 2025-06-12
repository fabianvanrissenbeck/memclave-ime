.text
.globl ime_lock_memory
.globl ime_unlock_memory
.globl ime_wipe_user

ime_lock_memory:
    jump r23

ime_unlock_memory:
    jump r23

ime_wipe_user:
    xor zero, r17, 0, z, ime_no_wipe

    move r0, __ime_wram_end - 4
wipe_loop:
    sw r0, 0, 0
    sub r0, r0, 4, pl, wipe_loop

ime_no_wipe:
    jump r23