#include "ime.h"

#include <defs.h>
#include <stddef.h>
#include <stdint.h>

typedef enum ime_mram_msg_type {
    IME_MRAM_MSG_NOP,
    IME_MRAM_MSG_WAITING,
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

// __mram_noinit contains __attribute__((used)) which triggers a warning because
// that attribute is useless in combination with extern
#pragma GCC push
#pragma GCC diagnostic ignored "-Wignored-attributes"
extern ime_mram_msg __mram_noinit __ime_msg_buf;
#pragma GCC pop

int main(void) {
    __ime_msg_buf.type = IME_MRAM_MSG_WAITING;
    __ime_wait_for_host();

    switch (__ime_msg_buf.type) {
    case IME_MRAM_MSG_PING:
        __ime_msg_buf.type = IME_MRAM_MSG_PONG;
        break;

    case IME_MRAM_MSG_READ_WRAM:
        __ime_msg_buf.wram.value = *((uint32_t*) __ime_msg_buf.wram.addr);
        __ime_msg_buf.type = IME_MRAM_MSG_NOP;
        break;

    case IME_MRAM_MSG_WRITE_WRAM:
        *((uint32_t*) __ime_msg_buf.wram.addr) = __ime_msg_buf.wram.value;
        __ime_msg_buf.type = IME_MRAM_MSG_NOP;
        break;

    case IME_MRAM_MSG_LOAD_SK:
        __ime_msg_buf.type = IME_MRAM_MSG_NOP;
        __ime_replace_sk((void __mram_ptr*) __ime_msg_buf.load.ptr, NULL, NULL, 1);
        break;

    default:
        break;
    }
}