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

void poly_feed(poly_context* ctx, size_t n, const uint32_t data[n], uint32_t out[4]);

void poly_free(poly_context* ctx);

#if POLY_DEBUG

void poly_blind_reduce(uint32_t target[5]);
void poly_mul_key(poly_context* ctx);

#endif

#endif
