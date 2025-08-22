#include "poly.h"

#include <stdbool.h>

extern void poly_masked_reduce(uint32_t target[5], uint32_t mask);

extern void poly_add_assign_4(uint32_t target[4], const uint32_t source[4]);

extern void poly_add_reduce(uint32_t target[5], const uint32_t block[5]);

extern void poly_mul_assign(uint32_t a[5], const uint32_t r[4]);

/** return UINT32_MAX if n is larger or equal to (1 << 130) - 5 and 0 otherwise */
static uint32_t poly_reduction_req(const uint32_t n[5]) {
    bool a = n[4] > 0x3;
    bool b = n[4] == 0x3;
    bool c = n[3] == UINT32_MAX && n[2] == UINT32_MAX && n[1] == UINT32_MAX;
    bool d = n[0] >= 0xFFFFFFFB;

    bool res = a | (b & c & d);
    return 0 - (uint32_t) res;
}

/** subtract the prime (1 << 130) - 5 from target if target is larger than it */
static void poly_blind_reduce(uint32_t target[5]) {
    uint32_t mask = poly_reduction_req(target);
    poly_masked_reduce(target, mask);
}

static void poly_mul_key(poly_context* ctx) {
    poly_mul_assign(&ctx->a[0], &ctx->r[0]);
    poly_blind_reduce(&ctx->a[0]);
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
    poly_add_assign_4(ctx->a, ctx->s);

    for (int i = 0; i < 4; ++i) {
        tag[i] = ctx->a[i];
    }

    *ctx = (poly_context) { 0 };
}
