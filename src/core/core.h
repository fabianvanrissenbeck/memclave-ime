#ifndef CORE_H
#define CORE_H

#include <defs.h>
#include <stdint.h>

/**
 * @brief replace the old subkernel saving its output data with a new subkernel initialised with that data
 *
 * The following steps are executed:
 *
 * 1. Switch to system mode loading the address to fetch the subkernel from. (Done)
 * 2. Check that no other threads are running. (Done)
 * 3. Clean WRAM from user data. (Done)
 * 3. Lock MRAM.
 * 4. Decrypt and verify in MRAM. (protect decryption against >1 thread)
 * 5. Load into WRAM. (Done)
 * 6. Load into IRAM. (Done)
 * 7. Unlock MRAM.
 * 8. Return to user mode, invoke subkernel. (Done)
 */
_Noreturn void core_replace_sk(void);

#endif
