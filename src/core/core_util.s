.text
.globl ime_check_system
.globl ime_sec_fault
.globl ime_sanity_fault

ime_check_system:
    lw r1, zero, 0
    lw r2, zero, 4
    ldma zero, r21, 0
    sw zero, 0, r1
    sw zero, 4, r2

    move r1, id
    add zero, zero, r1, nz, ime_sec_fault

    jump r0

ime_sec_fault:
    fault 0x101011

ime_sanity_fault:
    fault 0x101012