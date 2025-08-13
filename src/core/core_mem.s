.text
.globl ime_lock_memory
.globl ime_unlock_memory
.globl ime_wipe_user

ime_lock_memory:
    jump r23

ime_unlock_memory:
    jump r23

// r17 contains pointer to first location not to wipe - use as counter
ime_wipe_user:
    move r0, r17

ime_wipe_loop:
    add r0, r0, -4, mi, ime_wipe_done
    sw r0, 0x0, 0x0
    jump ime_wipe_loop

ime_wipe_done:
    jump r23
