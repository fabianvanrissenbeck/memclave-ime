#include "aead.h"
#include "ime.h"

#include <assert.h>

uint32_t key[8] = { 0 };
uint32_t buf[4] = { 0 };
uint32_t tag[4];
uint32_t iv[3] = { 0 };

int main(void) {
    ime_aead_enc(key, iv, sizeof(buf), buf, buf, tag, iv);
    assert(ime_aead_dec(key, iv, tag, sizeof(buf), buf, buf));

    for (int i = 0; i < 4; ++i) {
        __ime_debug_out[i] = buf[i];
        __ime_debug_out[i + 4] = tag[i];

        if (i < 3) {
            __ime_debug_out[i + 8] = iv[i];
        }
    }

    asm("stop");
}