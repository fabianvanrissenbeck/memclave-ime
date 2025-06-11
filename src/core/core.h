#ifndef IME_CORE_H
#define IME_CORE_H

#include <defs.h>
#include <stdint.h>
#include <stdnoreturn.h>

/**
 * @brief replace the current subkernel terminating it and executing the new one
 * @param sk address in MRAM where the subkernel is located in encrypted form
 * @param tag expected tag of the subkernel to load - used to enforce execution of specific subkernel
 * @param user_key key used to verify the subkernel or NULL to use the system key
 */
void noreturn ime_replace_sk(void __mram_ptr* sk, const uint8_t tag[12], const uint8_t user_key[32]);

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

/** load the IRAM portion of the subkernel */
void ime_load_iram(void);

#endif
