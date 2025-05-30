#ifndef CORE_H
#define CORE_H

#include <defs.h>
#include <stdint.h>

/**
 * @brief replace the old subkernel saving its output data with a new subkernel initialised with that data
 *
 * The following steps are executed:
 *
 * 1. Switch to system mode loading the address to fetch the subkernel from
 * 2. Check that no other threads are running, return to user on failure.
 * 3. Clean WRAM and IRAM from user data.
 * 3. Lock MRAM.
 * 4. Decrypt and verify in MRAM. (protect decryption against >1 thread)
 * 5. Verify and load into IRAM.
 * 6. Load into WRAM.
 * 7. Unlock MRAM.
 * 8. Return to user mode, invoke subkernel.
 */
_Noreturn void core_replace_sk(void);

#endif
