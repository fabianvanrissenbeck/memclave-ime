#ifndef AEAD_H
#define AEAD_H

#include "core.h"

#include <mram.h>
#include <stdint.h>
#include <stdbool.h>

_Static_assert(__builtin_offsetof(ime_sk, tag) == 4, "bad struct alignment");
_Static_assert(__builtin_offsetof(ime_sk, iv) == 20, "bad struct alignment");

bool ime_decrypt_verify(ime_sk __mram_ptr* sk);

#endif
