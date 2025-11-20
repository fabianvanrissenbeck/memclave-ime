/**
 * poly.h - Reduced Poly1305 implementation used within the TL and by user code.
 *
 * The library is linked into the TL, but trampolines are provided that jump to
 * the 3 exposed functions. Since poly does not access any confidential data itself,
 * this should be fine. The values of r20 and r21 are invalid when jumping here
 * from user mode.
 */

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

#endif
