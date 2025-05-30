#ifndef BOOT_H
#define BOOT_H

#include <stdint.h>

/**
 * @brief Resume all threads.
 *
 * This function should only be called from thread 0 and if all threads are stopped on __thread_stop.
 * It resumes threads 1 to NR_TASKLETS, causing them to reset their stack pointer and call main.
 *
 * @returns 0 on success or -1 if not called from thread 0
 */
extern int thrd_boot_all(void);

/**
 * @brief Get a bitmap showing which threads are active.
 *
 * This function boots up all threads from 1 to NR_TASKLETS at PC=0. Because of the code placed at __bootstrap,
 * all threads that were inactive boot up and almost immediately execute a stop instruction, stopping them at
 * __thread_stop and clearing their RUN-bit. Threads that were already running usually do not reset their PC to 0
 * (sometimes they do causing them to stop), which means their RUN-bit is definitely high after executing the
 * boot instruction. The RUN-bits are then inspected using a second invocation of the boot instruction. Terminated
 * threads quickly rerun and terminate again.
 *
 * @return A bitmap where (res & (1 << tid)) iff tid is active. 0 if called from any thread other than 0.
 */
extern uint16_t thrd_get_state(void);

#endif