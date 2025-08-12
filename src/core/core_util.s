.text
.globl ime_check_system
.globl ime_sec_fault
.globl ime_sanity_fault
.globl ime_push_sk
.globl ime_pop_sk

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

// void ime_copy_sk(ime_sk __mram_ptr* src, ime_sk __mram_ptr* dst)
ime_copy_sk:
    lw r2, zero, 0x0
    lw r3, zero, 0x4

    add r0, r0, 0x20
    ldma zero, r0, 0x0
    add r0, r0, -0x20

    lw r4, zero, 0x4 // src->size
ime_copy_sk_loop:
    ldma zero, r0, 0x0
    sdma zero, r1, 0x0

    add r0, r0, 0x8
    add r1, r1, 0x8
    add r4, r4, -0x8, nz, ime_copy_sk_loop

    sw zero, 0x0, r2
    sw zero, 0x4, r3

    jump r23

ime_push_sk:
    move r1, __ime_scratch_a
    jump ime_copy_sk

ime_pop_sk:
    move r1, r0
    move r0, __ime_scratch_a

    jump ime_copy_sk
