#include "ime.h"

#include <mram.h>
#include <defs.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#define IME_MESSAGE_BUFFER ((ime_mram_msg __mram_ptr*) ((63 << 20) + (512 << 10)))

/* MRAM Memory Layout
 *
 * 0-63 MiB (unused)
 * 63 MiB + 000 KiB - Messaging Subkernel
 * 63 MiB + 128 KiB - Key Exchange Subkernel
 * 63 MiB + 512 KiB - Message Buffer
 */

typedef enum ime_mram_msg_type {
    IME_MRAM_MSG_NOP,
    IME_MRAM_MSG_PING,
    IME_MRAM_MSG_PONG,
    IME_MRAM_MSG_READ_WRAM,
    IME_MRAM_MSG_WRITE_WRAM,
    IME_MRAM_MSG_LOAD_SK,
} ime_mram_msg_type;

typedef struct ime_mram_msg {
    uint32_t type;
    union {
        struct {
            uint32_t addr;
            uint32_t value;
        } wram;
        struct {
            uint32_t ptr;
        } load;
        struct {
            uint32_t pad[3];
        };
    };
} ime_mram_msg;

_Static_assert(__builtin_offsetof(ime_mram_msg, wram.addr) == 4, "incorrect alignment");
_Static_assert(__builtin_offsetof(ime_mram_msg, wram.value) == 8, "incorrect alignment");

int main(void) {
    uint64_t __mram_ptr* ctr = ((uint64_t __mram_ptr*) IME_MESSAGE_BUFFER) + 2;

    __ime_wait_for_host();
    ctr[0] += 1;

    switch (IME_MESSAGE_BUFFER->type) {
    case IME_MRAM_MSG_PING:
        IME_MESSAGE_BUFFER->type = IME_MRAM_MSG_PONG;
        break;

    case IME_MRAM_MSG_READ_WRAM:
        IME_MESSAGE_BUFFER->wram.value = *((uint32_t*) IME_MESSAGE_BUFFER->wram.addr);
        break;

    case IME_MRAM_MSG_WRITE_WRAM:
        *((uint32_t*) IME_MESSAGE_BUFFER->wram.addr) = IME_MESSAGE_BUFFER->wram.value;
        break;

    case IME_MRAM_MSG_LOAD_SK:
        __ime_replace_sk((void __mram_ptr*) IME_MESSAGE_BUFFER->load.ptr, NULL, NULL, 1);
        break;

    default:
        break;
    }

    /*
    switch (IME_MESSAGE_BUFFER->type) {
    case IME_MRAM_MSG_NOP:
        break;

    case IME_MRAM_MSG_READ_WRAM:
        IME_MESSAGE_BUFFER->wram.value = *((uint32_t*) IME_MESSAGE_BUFFER->wram.addr);
        break;

    case IME_MRAM_MSG_WRITE_WRAM:
        *((uint32_t*) IME_MESSAGE_BUFFER->wram.addr) = IME_MESSAGE_BUFFER->wram.value;
        break;

    default:
        break;
    }
    */
}