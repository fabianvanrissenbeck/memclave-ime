#include "core.h"
#include "aead.h"

void ime_decrypt_sk(ime_sk __mram_ptr* sk, const uint32_t tag[4], uint32_t user_key[8]) {
    IME_CHECK_SYSTEM();

    uint32_t sk_tag[4];
    mram_read(&sk->tag[0], sk_tag, sizeof(sk_tag));

    if (tag) {
        if (tag[0] != sk_tag[0] || tag[1] != sk_tag[1] || tag[2] != sk_tag[2] || tag[3] != sk_tag[3]) {
            ime_sec_fault();
        }
    }

    if (!ime_decrypt_verify(sk, user_key)) {
        ime_sec_fault();
    }
}
