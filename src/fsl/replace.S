.section .replace
.globl replace

replace:
    move r0, 0x0
    move r1, g_tl - 0x08000000 // has to be converted from elf to DPU format

    ldmai r0, r1, 255 // load in 2 KiB chunks
    add r0, r0, 2048
    add r1, r1, 2048

    ldmai r0, r1, 255
    add r0, r0, 2048
    add r1, r1, 2048

    ldmai r0, r1, 255
    add r0, r0, 2048
    add r1, r1, 2048

    ldmai r0, r1, 255
    add r0, r0, 2048
    add r1, r1, 2048

    move r0, g_sk_msg - 0x08000000
    move r1, 0x0 // TODO: should enforce correct tag
    move r2, sys_key // TODO: should use system key
    move r3, 0x0 // TODO: should WIPE everything
    move r23, 0x0 // just to make sure

    jump 0x0