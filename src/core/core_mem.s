.text
.globl ime_wipe_user

// r17 contains pointer to first location not to wipe - use as counter
ime_wipe_user:
    // move r0, r17
    add r0, r17, -4, mi, ime_wipe_done

ime_wipe_loop:
    sw r0, 0x0, 0x0
    add r0, r0, -4, pl, ime_wipe_loop

ime_wipe_done:
    jump r23

/*
ime_wipe_loop:
    add r0, r0, -4, mi, ime_wipe_done
    sw r0, 0x0, 0x0
    jump ime_wipe_loop

ime_wipe_done:
    jump r23

*/