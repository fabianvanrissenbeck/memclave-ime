#include "poly.h"

#include <stddef.h>
#include <stdbool.h>

#if POLY_DEBUG
#define STATIC
#else
#define STATIC static
#endif

STATIC void poly_masked_reduce(uint32_t target[5], uint32_t mask) {
    bool carry = false;

    for (int i = 0; i < 5; ++i) {
        uint32_t n;

        if (i == 0) {
            n = 5;
        } else if (i == 4) {
            n = 0xFFFFFFFC;
        } else {
            n = 0;
        }

        bool new_carry = (uint32_t)(target[i] + n) < n;

        target[i] += (n + carry) & mask;
        carry = new_carry;
    }
}

/** return UINT32_MAX if n is larger or equal to (1 << 130) - 5 and 0 otherwise */
STATIC uint32_t poly_reduction_req(const uint32_t n[5]) {
    bool a = n[4] > 0x3;
    bool b = n[4] == 0x3;
    bool c = n[3] == UINT32_MAX && n[2] == UINT32_MAX && n[1] == UINT32_MAX;
    bool d = n[0] >= 0xFFFFFFFB;

    bool res = a | (b & c & d);
    return 0 - (uint32_t) res;
}

STATIC void poly_fast_reduce(uint32_t target[5]) {
    uint32_t mask = 0 - (target[4] > 3);
    poly_masked_reduce(target, mask);
}

/** subtract the prime (1 << 130) - 5 from target if target is larger than it */
STATIC void poly_blind_reduce(uint32_t target[5]) {
    uint32_t mask = poly_reduction_req(target);
    poly_masked_reduce(target, mask);
}

STATIC bool poly_add_assign(size_t n, uint32_t target[n], const uint32_t source[n]) {
    bool carry = false;

    for (size_t i = 0; i < n; ++i) {
        uint64_t val = (uint64_t) target[i] + (uint64_t) source[i] + (uint64_t) carry;

        target[i] = val & UINT32_MAX;
        carry = (val >> 32) != 0;
    }

    return carry;
}

/** add a block of data to the target and reduce it below the prime */
STATIC void poly_add_reduce(uint32_t target[5], const uint32_t block[5]) {
    poly_add_assign(5, target, block);
    // poly_blind_reduce(target);
    poly_fast_reduce(target);
}

/** multiply the target by two and reduce it below the prime */
STATIC void poly_double_reduce(uint32_t target[5]) {
    target[4] = target[4] << 1 | target[3] >> 31;
    target[3] = target[3] << 1 | target[2] >> 31;
    target[2] = target[2] << 1 | target[1] >> 31;
    target[1] = target[1] << 1 | target[0] >> 31;
    target[0] = target[0] << 1;

    // poly_blind_reduce(target);
    poly_fast_reduce(target);
}

STATIC void poly_mul_key(poly_context* ctx) {
    uint32_t prod[5] = { 0 };

    for (int i = 0; i < 128; ++i) {
        if ((ctx->r[i / 32] >> (i % 32)) & 0x1) {
            poly_add_reduce(prod, ctx->a);
        }

        poly_double_reduce(ctx->a);
    }

    poly_blind_reduce(prod);

    for (int i = 0; i < 5; ++i) { ctx->a[i] = prod[i]; }
}

void poly_feed_block(poly_context* ctx, const uint32_t block[4]) {
    uint32_t input[5] = {
        block[0], block[1], block[2], block[3], 0x1
    };

    poly_add_reduce(ctx->a, input);
    poly_mul_key(ctx);
}

void poly_init(poly_context* ctx, const uint32_t key[8]) {
    *ctx = (poly_context) {
        .r = {
            key[0] & 0x0FFFFFFF,
            key[1] & 0x0FFFFFFC,
            key[2] & 0x0FFFFFFC,
            key[3] & 0x0FFFFFFC,
        },
        .s = {
            key[4], key[5], key[6], key[7],
        }
    };
}

void poly_finalize(poly_context* ctx, uint32_t tag[4]) {
    poly_add_assign(4, ctx->a, ctx->s);

    for (int i = 0; i < 4; ++i) {
        tag[i] = ctx->a[i];
    }

    *ctx = (poly_context) { 0 };
}
