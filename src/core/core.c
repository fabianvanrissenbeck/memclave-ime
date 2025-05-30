#include "core.h"
#include "boot.h"

#include <defs.h>
#include <mram.h>
#include <mutex.h>

#define SECFAULT() asm("fault 0x101010")

#define SECASSERT(expr) do { \
    if (!(expr)) { \
        SECFAULT(); \
    } \
} while (0)

extern void core_enter_system(void);

extern void core_leave_system(void);

extern void core_load_iram(void);

extern void core_load_wram(void);

#define core_perform_tailcall(func) \
do { \
    __asm__ volatile( \
        "jump " #func "\n" \
    ); \
    __builtin_unreachable(); \
} while (0)

#if 0
__attribute__((always_inline))
static _Noreturn void core_perform_tailcall(void (*func)(void)) {
    __asm__ volatile(
        "jump %0\n"
        :
        : "r" (func)
    );

    __builtin_unreachable();
}
#endif

/**
 * Decrypt and verify performs decryption and full verification of the subkernel.
 */
void core_decrypt_and_verify(void) {
    core_perform_tailcall(core_load_wram);
}

void core_lock_mram(void) {
    core_perform_tailcall(core_decrypt_and_verify);
}

/** clear everything after the system stack - the stack itself has to be cleared seperately */
void core_clear_memory(void) {
    extern uint32_t __ime_user_start_w[];
    extern uint32_t __ime_wram_end[];

    for (volatile uint32_t* cur = &__ime_user_start_w[0]; cur != &__ime_wram_end[0]; cur++) {
        *cur = 0xFFFFFFFF;
    }

    core_perform_tailcall(core_lock_mram);
}

void core_check_threads(void) {
    SECASSERT(me() == 0);
    SECASSERT(thrd_get_state() == 0x0001);

    core_perform_tailcall(core_clear_memory);
}

void core_finalize_load(void) {
    extern uint32_t __ime_persistent_end[];
    extern uint32_t __ime_user_start_w[];

    for (volatile uint32_t* cur = &__ime_persistent_end[0]; cur != &__ime_user_start_w[0]; ++cur) {
        *cur = 0xFFFFFFFF;
    }

    core_perform_tailcall(core_leave_system);
}

void core_unlock_mram(void) {
    core_perform_tailcall(core_finalize_load);
}

_Noreturn void core_replace_sk(void) {
    core_perform_tailcall(core_enter_system);
}
