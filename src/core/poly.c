#include "poly.h"

#include <stdbool.h>

extern void poly_masked_reduce(uint32_t target[5], uint32_t mask);

extern void poly_add_reduce(uint32_t target[5], const uint32_t block[5]);

extern void poly_mul_assign(uint32_t a[5], const uint32_t r[4]);

extern void poly_add_128(uint32_t target[4], uint32_t a[4], uint32_t b[4]);

/** return UINT32_MAX n is larger than (1 << 130) - 5 and 0 otherwise - assumes n < 2^130 */
static uint32_t poly_reduction_req(const uint32_t n[5]) {
    bool b = n[4] == 0x3;
    bool c = n[3] == UINT32_MAX && n[2] == UINT32_MAX && n[1] == UINT32_MAX;
    bool d = n[0] >= 0xFFFFFFFB;

    bool res = (b & c & d);
    return 0 - (uint32_t) res;
}

static void poly_mul_key(poly_context* ctx) {
    poly_mul_assign(&ctx->a[0], &ctx->r[0]);

    // values may be between p and 2^130 - 1 and are not reduced in that case
    uint32_t mask = poly_reduction_req(&ctx->a[0]);
    poly_masked_reduce(&ctx->a[0], mask);
}

void poly_feed_block(poly_context* ctx, const uint32_t block[4]) {
    poly_add_reduce(ctx->a, block);
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
    poly_add_128(tag, ctx->a, ctx->s);
    *ctx = (poly_context) { 0 };
}
