#include "core.h"
#include "aead.h"

void ime_decrypt_sk(void __mram_ptr* sk, const uint8_t tag[12], uint8_t user_key[32]) {
    IME_CHECK_SYSTEM();
    ime_decrypt_verify(sk);
}
