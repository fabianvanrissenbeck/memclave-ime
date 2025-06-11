.text
.globl ime_check_threads

ime_check_threads:
    /* boot up all threads setting the RUN_BIT[i] state to 1 */
    move r0, NR_TASKLETS - 1
loop_1:
    boot r0, 0, false, 0x0
    sub r0, r0, 1, nz, loop_1

    /* give the threads some time to boot and terminate again setting RUN_BIT[i] to zero */
    move r0, 10000
loop_2:
    sub r0, r0, 1, nz, loop_2

    /* inspect the RUN_BIT[i] state by booting the threads again */
    move r0, NR_TASKLETS - 1
loop_3:
    boot r0, 0, nz, ime_sec_fault
    sub r0, r0, 1, nz, loop_3

    /* give the threads some time to boot and terminate */
    move r0, 10000
loop_4:
    sub r0, r0, 1, nz, loop_4

    jump r23

ime_sec_fault:
    fault 0x101010