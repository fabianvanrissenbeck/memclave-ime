#ifndef IME_CORE_H
#define IME_CORE_H

#include <defs.h>
#include <stdint.h>
#include <stdnoreturn.h>

#define IME_CHECK_SYSTEM() \
do { \
    __asm__ volatile( \
        "call r0, ime_check_system\n" \
        : \
        : \
        : "r0", "r1", "r2", "memory" \
    ); \
} while (0)

typedef struct ime_sk {
    uint32_t magic;
    uint32_t tag[4];
    uint32_t iv[3];
    uint32_t size_aad;
    uint32_t size;
    uint32_t text_size;
    uint32_t data_size;
    uint32_t pad[4];
    uint64_t body[];
} ime_sk;

/**
 * @brief replace the current subkernel terminating it and executing the new one
 * @param sk address in MRAM where the subkernel is located in encrypted form
 * @param tag expected tag of the subkernel to load - used to enforce execution of specific subkernel
 * @param user_key key used to verify the subkernel or NULL to use the system key
 * @param wipe_wram if not zero wipe all of wram before proceeding with the load process
 */
void noreturn ime_replace_sk(void __mram_ptr* sk, const uint8_t tag[16], const uint8_t user_key[32], uint32_t wipe_wram);

/** cause a security violation if any threads is running */
void ime_check_threads(void);

/** fully wipe WRAM from user data excluding the user key which is wiped later */
void ime_wipe_user(void);

/** lock MRAM from external accesses */
void ime_lock_memory(void);

/** unlock MRAM from external accesses */
void ime_unlock_memory(void);

/**
 * @brief decrypt the subkernel in place in MRAM and verify its authenticity
 * @param sk pointer to the subkernel structure
 * @param tag expected tag - fails if any other tag is found instead
 * @param user_key key to use - may be NULL to use the system key - wiped after use
 */
void ime_decrypt_sk(void __mram_ptr* sk, const uint8_t tag[12], uint8_t user_key[32]);

/**
 * @brief scan the subkernel for any banned instructions - fault if any are found
 * @param sk pointer to the subkernel structure
 */
void ime_scan_sk(void __mram_ptr* sk);

/** load the MRAM portion of the subkernel */
void ime_load_wram(void);

/** load the IRAM portion of the subkernel - returns load address */
void __mram_ptr* ime_load_iram(void);

/** copy the passed subkernel to __ime_scratch_a */
void ime_push_sk(const ime_sk __mram_ptr* sk);

/** restore the passed subkernel from __ime_scratch_a */
void ime_pop_sk(ime_sk __mram_ptr* sk);

/**
 * @brief check that the current thread is in system mode
 *
 * This function does not follow UPMEM's calling convention so that it can
 * be called from within the loading subroutines without using WRAM. It
 * jumps to register 0 (first function parameter) after finishing.
 *
 */
void ime_check_system(int (*func)());

/** fault triggered on security related issues */
void noreturn ime_sec_fault(void);

/** fault triggered when a fatal but not security related issue occurs */
void noreturn ime_sanity_fault(void);

#endif
