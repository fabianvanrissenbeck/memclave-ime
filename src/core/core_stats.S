/**
  * IME stats reporting
  *
  * Stats are reported in the __ime_debug_out section of MRAM. This section is treated
  * as an array of perfcounter outputs. Each time ime_stats_here is called, the
  * first value of the array is incremented by one, pointing to the next free cell.
  * The prior free cell is filled with the perfcounter output.
  *
  * ime_stats_here assumes a valid stack pointer and 64 bytes to spare
*/

#if IME_REPORT_STATS == 1
.text
.globl ime_stats_here

ime_stats_here:
    time r0

    move r1, __ime_debug_out
    ldma r22, r1, 7

    lw r2, r22, 0x3C
    and r2, r2, 0x3C
    add r3, r2, 0x4
    sw r22, 0x3C, r3

    add r2, r2, r22
    sw r2, 0x0, r0

    sdma r22, r1, 7
    jump r23

#endif