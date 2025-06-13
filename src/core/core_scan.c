#include "core.h"

#include <mram.h>
#include <stddef.h>
#include <stdbool.h>

static bool is_ldmai(uint64_t n) {
    if (n >> 39 != 0xe0) { return false; }
    if ((n >> 20 & 0xF) != 0) { return false; }
    if ((n & 0x1FF) != 0x1) { return false; }

    return true;
}

static bool is_thread_ctrl(uint64_t n) {
    if (n >> 41 != 0x3e) { return false; }
    if ((n >> 39 & 0x3) == 0) { return false; }
    if ((n >> 28 & 0x3F) != 0x32) { return false; }

    return true;
}

static bool is_banned_reg(uint64_t n) {
    uint8_t wr = n >> 39 & 0x1F;

    if (wr < 24) {
        return wr == 20 || wr == 21;
    }

    wr = (n >> 44 & 0x3) << 3 | (n >> 39 & 0x7);
    return wr == 20 || wr == 21;
}

void ime_scan_sk(void __mram_ptr* sk_ptr) {
    // we cannot assume a stack pointer being in place and at a good position
    // set it manually to zero. User code will have to avoid the first section in wram
    // for data persisting over multiple subkernels.
    const ime_sk __mram_ptr* sk = sk_ptr;
    uint64_t sz;

    mram_read(&sk->text_size, &sz, sizeof(sz));

    // the lower 32-bits contain the value of sk->text_size
    for (size_t i = 0; i < (sz & UINT32_MAX) * 256; ++i) {
        uint64_t inst;
        mram_read(&sk->body[i], &inst, sizeof(inst));

        if (is_ldmai(inst)) { ime_sec_fault(); }
        if (is_thread_ctrl(inst)) { ime_sec_fault(); }
        if (is_banned_reg(inst)) { ime_sec_fault(); }
    }

    IME_CHECK_SYSTEM();
    return;
}