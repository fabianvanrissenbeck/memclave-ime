#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include <stddef.h>

typedef struct poly_context {
    uint32_t r[4];
    uint32_t s[4];
    uint32_t a[5];
} poly_context;

void poly_init(poly_context* ctx, const uint32_t key[8]);

void poly_feed_block(poly_context* ctx, const uint32_t block[4]);

void poly_finalize(poly_context* ctx, uint32_t out[4]);

#if POLY_DEBUG

void poly_blind_reduce(uint32_t target[5]);
void poly_mul_key(poly_context* ctx);

#endif

#endif
