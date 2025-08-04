#ifndef AEAD_H
#define AEAD_H

#include <mram.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct ime_sk {
    uint32_t magic;
    uint32_t tag[4];
    uint32_t iv[3];
    uint32_t size_aad;
    uint32_t size;
    uint32_t text_size;
    uint32_t data_size;
    uint32_t pad[4];
    uint64_t body[];
} ime_sk;

_Static_assert(__builtin_offsetof(ime_sk, tag) == 4, "bad struct alignment");
_Static_assert(__builtin_offsetof(ime_sk, iv) == 20, "bad struct alignment");

bool ime_decrypt_verify(ime_sk __mram_ptr* sk);

#endif
