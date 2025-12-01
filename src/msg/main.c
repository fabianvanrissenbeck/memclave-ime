#include "ime.h"

#include <defs.h>
#include <stddef.h>
#include <stdint.h>

typedef enum ime_mram_msg_type {
    IME_MRAM_MSG_NOP,
    IME_MRAM_MSG_WAITING,
    IME_MRAM_MSG_PING,
    IME_MRAM_MSG_PONG,
    IME_MRAM_MSG_LOAD_SK,
} ime_mram_msg_type;

typedef enum ime_load_sk_flags {
    IME_LOAD_SK_USER_KEY = 1 << 0,
} ime_load_sk_flags;

typedef union ime_mram_msg {
    struct {
        uint16_t type;
        uint16_t flags;
        uint32_t ptr;
    };
    uint64_t raw;
} ime_mram_msg;

_Static_assert(__builtin_offsetof(ime_mram_msg, type) == 0, "incorrect struct alignment");
_Static_assert(__builtin_offsetof(ime_mram_msg, flags) == 2, "incorrect struct alignment");
_Static_assert(__builtin_offsetof(ime_mram_msg, ptr) == 4, "incorrect struct alignment");
_Static_assert(sizeof(ime_mram_msg) == 8, "MRAM message should be 8 bytes large");

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

    case IME_MRAM_MSG_LOAD_SK: {
        __ime_msg_buf.type = IME_MRAM_MSG_NOP;

        void __mram_ptr* load_addr = (void __mram_ptr*) __ime_msg_buf.ptr;
        uint32_t* key = (__ime_msg_buf.flags & IME_LOAD_SK_USER_KEY) ? g_load_prop.key : NULL;

        __ime_replace_sk(load_addr, NULL, key);
        __builtin_unreachable();
    }


    default:
        break;
    }
}