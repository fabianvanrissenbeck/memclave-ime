#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <defs.h>
#include <barrier.h>
#include <perfcounter.h>

#define DHKE_KEY_BITS 2048
#define DHKE_KEY_BYTES (DHKE_KEY_BITS / 8)
#define DHKE_KEY_WORDS (DHKE_KEY_BYTES / sizeof(uint32_t))

typedef struct bigint {
    union {
        uint8_t bytes[DHKE_KEY_BYTES];
        uint32_t words[DHKE_KEY_WORDS];
    };
} bigint;

BARRIER_INIT(barrier_init, 16)
BARRIER_INIT(barrier_aggr, 16)
BARRIER_INIT(barrier_done, 16)

static const __host bigint private_key = {
    .bytes = {
        0x04, 0xe8, 0xa0, 0x72, 0xd3, 0xe5, 0x9e, 0xdb,
        0x70, 0x56, 0xfe, 0x81, 0x03, 0x3b, 0xc2, 0x5f,
        0x04, 0xe8, 0xa0, 0x72, 0xd3, 0xe5, 0x9e, 0xdb,
        0x70, 0x56, 0xfe, 0x81, 0x03, 0x3b, 0xc2, 0x5f
    }
};

__attribute__((used))
static uint64_t runtime;

bigint prime = {
    .bytes = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xAD, 0xF8, 0x54, 0x58, 0xA2, 0xBB, 0x4A, 0x9A,
        0xAF, 0xDC, 0x56, 0x20, 0x27, 0x3D, 0x3C, 0xF1,
        0xD8, 0xB9, 0xC5, 0x83, 0xCE, 0x2D, 0x36, 0x95,
        0xA9, 0xE1, 0x36, 0x41, 0x14, 0x64, 0x33, 0xFB,
        0xCC, 0x93, 0x9D, 0xCE, 0x24, 0x9B, 0x3E, 0xF9,
        0x7D, 0x2F, 0xE3, 0x63, 0x63, 0x0C, 0x75, 0xD8,
        0xF6, 0x81, 0xB2, 0x02, 0xAE, 0xC4, 0x61, 0x7A,
        0xD3, 0xDF, 0x1E, 0xD5, 0xD5, 0xFD, 0x65, 0x61,
        0x24, 0x33, 0xF5, 0x1F, 0x5F, 0x06, 0x6E, 0xD0,
        0x85, 0x63, 0x65, 0x55, 0x3D, 0xED, 0x1A, 0xF3,
        0xB5, 0x57, 0x13, 0x5E, 0x7F, 0x57, 0xC9, 0x35,
        0x98, 0x4F, 0x0C, 0x70, 0xE0, 0xE6, 0x8B, 0x77,
        0xE2, 0xA6, 0x89, 0xDA, 0xF3, 0xEF, 0xE8, 0x72,
        0x1D, 0xF1, 0x58, 0xA1, 0x36, 0xAD, 0xE7, 0x35,
        0x30, 0xAC, 0xCA, 0x4F, 0x48, 0x3A, 0x79, 0x7A,
        0xBC, 0x0A, 0xB1, 0x82, 0xB3, 0x24, 0xFB, 0x61,
        0xD1, 0x08, 0xA9, 0x4B, 0xB2, 0xC8, 0xE3, 0xFB,
        0xB9, 0x6A, 0xDA, 0xB7, 0x60, 0xD7, 0xF4, 0x68,
        0x1D, 0x4F, 0x42, 0xA3, 0xDE, 0x39, 0x4D, 0xF4,
        0xAE, 0x56, 0xED, 0xE7, 0x63, 0x72, 0xBB, 0x19,
        0x0B, 0x07, 0xA7, 0xC8, 0xEE, 0x0A, 0x6D, 0x70,
        0x9E, 0x02, 0xFC, 0xE1, 0xCD, 0xF7, 0xE2, 0xEC,
        0xC0, 0x34, 0x04, 0xCD, 0x28, 0x34, 0x2F, 0x61,
        0x91, 0x72, 0xFE, 0x9C, 0xE9, 0x85, 0x83, 0xFF,
        0x8E, 0x4F, 0x12, 0x32, 0xEE, 0xF2, 0x81, 0x83,
        0xC3, 0xFE, 0x3B, 0x1B, 0x4C, 0x6F, 0xAD, 0x73,
        0x3B, 0xB5, 0xFC, 0xBC, 0x2E, 0xC2, 0x20, 0x05,
        0xC5, 0x8E, 0xF1, 0x83, 0x7D, 0x16, 0x83, 0xB2,
        0xC6, 0xF3, 0x4A, 0x26, 0xC1, 0xB2, 0xEF, 0xFA,
        0x88, 0x6B, 0x42, 0x38, 0x61, 0x28, 0x5C, 0x97,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    }
};

bigint inverse_prime;

static void init_inverse_prime(void) {
    bool carry = true;

    for (size_t i = 0; i < DHKE_KEY_WORDS; ++i) {
        uint32_t val = ~prime.words[i];

        if (carry) {
            val += carry;
            carry = val == 0;
        }

        inverse_prime.words[i] = val;
    }
}

static inline bool big_add_assign(bigint* v, const bigint* w) {
    bool carry = false;

#pragma unroll 16
    for (size_t i = 0; i < DHKE_KEY_WORDS; ++i) {
        v->words[i] += carry;
        carry = v->words[i] == 0;
        v->words[i] += w->words[i];
        carry |= v->words[i] < w->words[i];
    }

    return carry;
}

static void sub_prime_from(bigint* v) {
    big_add_assign(v, &inverse_prime);
}

static bool less_than_prime(const bigint* v) {
    for (int i = DHKE_KEY_WORDS - 1; i >= 0; --i) {
        if (v->words[i] > prime.words[i]) {
            return false;
        } else if (v->words[i] < prime.words[i]) {
            return true;
        }
    }

    return false;
}

static void mod_shift_left(bigint* v) {
    bool carry = false;

#pragma unroll 16
    for (size_t i = 0; i < DHKE_KEY_WORDS; ++i) {
        uint32_t old = v->words[i];

        v->words[i] = old << 1 | carry;
        carry = old >> 31 & 1;
    }

    if (carry | !less_than_prime(v)) {
        sub_prime_from(v);
    }
}

static void mod_add_assign(bigint* v, const bigint* w) {
    bool carry = big_add_assign(v, w);

    if (carry | !less_than_prime(v)) {
        sub_prime_from(v);
    }
}

static inline bool bit_at_pos(const bigint* v, size_t bit) {
    return v->words[bit / 32] >> (bit % 32) & 1;
}

static void mod_mul_assign(bigint* out, bigint* v, const bigint* w) {
    *out = (bigint) { 0 };

#pragma unroll 8
    for (size_t i = 0; i < DHKE_KEY_BITS; ++i) {
        if (bit_at_pos(w, i)) {
            mod_add_assign(out, v);
        }

        mod_shift_left(v);
    }
}

void dhke_compute_shared(bigint* out, const bigint* in) {
/* Algorithm in pseudocode
 *
 * tmp = in
 * res = 1
 *
 * for i in 0..3072
 *   if private_key.bits[i]
 *     res = (res * tmp) % prime
 *
 *   tmp = (tmp * 2) % prime
 *
 * This requires two primitives, modular left shift and modular multiplication.
 * Modular left shift is rather easy, the left shift is performed and if
 * the result is larger than the prime, or the result overflowed we subtract the
 * prime once. This is enough to compute the modulus, because a number n < p shifted
 * left by one is equal to 2 * n and 2 * n - p < 2 * p - p = p.
 *
 * The multiplication is more complex. We let m * n = n * 2^0 * m(0) + n * 2^1 * m(1) and so on.
 * For each step we need to store the current result, the original value of n, m and the current multiple of m.
 * While we can already create the multiples of m in the modular space, we cannot yet add values. Addition
 * however follows the same logic as left shifting, where if an overflow is detected or a value larger than p is
 * the result, we need to subtract p exactly once.
 */
    static bigint res[16];
    bigint tmp = *in;
    bigint scratch;

    for (size_t i = 0; i < (4 * 8 * 8 / 16) * me(); ++i) {
        mod_shift_left(&tmp);
    }

    res[me()] = (bigint) { 1 };

    for (size_t i = (4 * 8 * 8 / 16) * me(); i < (4 * 8 * 8 / 16) * (me() + 1); ++i) {
        if (bit_at_pos(&private_key, i)) {
            scratch = res[me()];
            mod_mul_assign(&res[me()], &scratch, &tmp);
        }

        mod_shift_left(&tmp);
    }

    barrier_wait(&barrier_aggr);

    if (me() == 0) {
        for (size_t i = 1; i < 16; ++i) {
            scratch = res[0];
            mod_mul_assign(&res[0], &scratch, &res[i]);
        }

        *out = res[0];
    }
}

int main(void) {
    static bigint in = {
        .words = {
            0xa89085ee, 0x37ad85ac, 0x63936540, 0x07117ee2,
            0xec6c3894, 0x4bf38154, 0xb37ffe70, 0xc1aa3a66,
            0xd50e5207, 0xfef0d699, 0x94c778e3, 0xe662027e,
            0xe9e62092, 0x445e4975, 0x9ce820d3, 0xc9064dd3,
            0xd43cb769, 0x398176d4, 0x7df30ef6, 0x2b95a282,
            0xe9fda97c, 0x1a4a8706, 0xbdd0118d, 0x3bd7a832,
            0xaa8f1ca7, 0x3378ac98, 0x417544e7, 0x5b023155,
            0xc9b9186f, 0xc0925042, 0xdc044638, 0x45929a48,
            0xdecdea95, 0x5d8082c7, 0x4621b8a0, 0x94beb14a,
            0x3b8869c7, 0xee0c264c, 0x2fb047a6, 0xbc36713b,
            0xefe15d0b, 0x2b804b52, 0x7ec6263c, 0xcb26e3c5,
            0xee39d0cd, 0xf4ca2269, 0x6e98c5c6, 0x62a949be,
            0x02d0b0ab, 0x90bf0eca, 0x64c3be2f, 0x8247e9dc,
            0xa988f710, 0x2b928866, 0x147678f8, 0x0f6df3c5,
            0x2147b7c5, 0x188c1b6a, 0xb1406dbf, 0xd61f1254,
            0xfecbcd3c, 0x9d12eb7e, 0x84cfc5eb, 0xada6a9d1,
        }
    };

    static bigint out;

    if (me() == 0) {
        init_inverse_prime();
        perfcounter_config(COUNT_CYCLES, true);
    }

    barrier_wait(&barrier_init);
    dhke_compute_shared(&out, &in);

    if (me() == 0) {
        runtime = perfcounter_get();
    }

    barrier_wait(&barrier_done);
}
