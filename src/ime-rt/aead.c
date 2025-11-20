#include "aead.h"
#include "poly.h"
#include "ime.h"

#include <string.h>

#define PTR_32_IS_MRAM(ptr) ((uintptr_t)(ptr) >> 31 & 0x1)

#define PTR_32_MRAM(ptr) (_Generic((ptr), \
    const t_ptr_32*: (const uint32_t __mram_ptr*)((uintptr_t)(ptr) & (~0x80000000)), \
    t_ptr_32*: (uint32_t __mram_ptr*)((uintptr_t)(ptr) & (~0x80000000))))

#define PTR_32(ptr) (_Generic((ptr), \
    const uint32_t*: ((const t_ptr_32*)(ptr)), \
    uint32_t*: ((t_ptr_32*)(ptr)), \
    const uint32_t __mram_ptr*: ((const t_ptr_32*)((uintptr_t)(ptr) | (1 << 31))), \
    uint32_t __mram_ptr*: ((t_ptr_32*)((uintptr_t)(ptr) | (1 << 31)))))

/** a pointer to either MRAM or WRAM
 *
 * Values of this type point to 32-bit unsigned integers in all cases.
 * If the leading bit of the pointer value is 1, the pointer is interpreted
 * as an MRAM address. Otherwise, it is interpreted as a WRAM address.
 *
 * Using this type, it is possible to write generic code that accesses
 * either MRAM or WRAM buffers, preventing duplicates.
 */
typedef struct { uint32_t a; } t_ptr_32;
_Static_assert(sizeof(t_ptr_32) == sizeof(uint32_t), "t_ptr_32 must have same properties as uint32_t");

/**
 * Read from a location referred to by a tagged pointer
 * @param src tagged pointer source
 * @param tgt target location - MUST be 8 byte aligned
 * @param sz size to read - MUST be a multiple of 8
 */
static void ptr_32_read(const t_ptr_32* src, uint32_t* tgt, size_t sz) {
    if (PTR_32_IS_MRAM(src)) {
        mram_read(PTR_32_MRAM(src), tgt, sz);
    } else {
        memcpy(tgt, src, sz);
    }
}

/**
 * Write to a location referred to by a tagged pointer
 * @param src source to read from - MUST be 8 byte aligned
 * @param tgt target location
 * @param sz size to write - MUST be a multiple of 8
 */
static void ptr_32_write(const uint32_t* src, t_ptr_32* tgt, size_t sz) {
    if (PTR_32_IS_MRAM(tgt)) {
        mram_write(src, PTR_32_MRAM(tgt), sz);
    } else {
        memcpy(tgt, src, sz);
    }
}

static void ime_chacha_crypt(const uint32_t* key, const uint32_t* iv,
                             size_t len, const t_ptr_32* buf, t_ptr_32* out_buf)
{
    __attribute__((aligned(8))) uint32_t block[16];
    __attribute__((aligned(8))) uint32_t tmp[16];

    for (size_t i = 0; i < (len + 63) / 64; ++i) {
        size_t sz = len - i * 16;
        sz = sz < 64 ? sz : 64;

        ptr_32_read(&buf[i * 16], tmp, sz);
        __ime_chacha_blk(key, i + 1, iv[0], iv[1], iv[2], block);

        for (size_t j = 0; j < 16; ++j) {
            tmp[j] ^= block[j];
        }

        ptr_32_write(tmp, &out_buf[i * 16], sz);
    }
}

static void ime_poly_simple(const uint32_t* key, const uint32_t* iv, size_t len, const t_ptr_32* buf, uint32_t* tag) {
    // must be 8 byte aligned - otherwise DMA transfers will not work (they ignore the last bits of the address)
    __attribute__((aligned(8))) uint32_t block[16];
    poly_context ctx;

    __ime_chacha_blk(key, 0, iv[0], iv[1], iv[2], block);
    poly_init(&ctx, block);

    for (size_t i = 0; i < len / 16; ++i) {
        ptr_32_read(&buf[i * 4], block, 16);
        poly_feed_block(&ctx, &block[0]);
    }

    poly_feed_block(&ctx, (uint32_t[4]) { 0, 0, len, 0 });
    poly_finalize(&ctx, tag);
}

static void ime_aead_enc_gen(const uint32_t* key, const uint32_t* p_iv,
                             size_t len, const t_ptr_32* buf, t_ptr_32* out_buf,
                             uint32_t* out_tag, uint32_t* out_iv)
{
    const uint32_t* iv = NULL;
    uint32_t iv_buf[4];

    if (p_iv) {
        iv = p_iv;
    } else {
        __ime_get_counter(iv_buf);
        iv = iv_buf;
    }

    ime_chacha_crypt(key, iv, len, buf, out_buf);
    ime_poly_simple(key, iv, len, out_buf, out_tag);

    if (out_iv) {
        out_iv[0] = iv[0];
        out_iv[1] = iv[1];
        out_iv[2] = iv[2];
    }
}

static bool ime_aead_dec_gen(const uint32_t* key, const uint32_t* iv, const uint32_t* tag,
                             size_t len, const t_ptr_32* buf, t_ptr_32* out_buf)
{
    bool valid = true;
    uint32_t computed_tag[4];

    ime_poly_simple(key, iv, len, buf, computed_tag);

    for (size_t i = 0; i < 4; ++i) {
        valid &= computed_tag[i] == tag[i];
    }

    if (!valid) { return false; }

    ime_chacha_crypt(key, iv, len, buf, out_buf);
    return true;
}

void ime_aead_enc(const uint32_t* key, const uint32_t* iv,
                  size_t len, const uint32_t* buf, uint32_t* out_buf,
                  uint32_t* out_tag, uint32_t* out_iv)
{
    ime_aead_enc_gen(key, iv, len, PTR_32(buf), PTR_32(out_buf), out_tag, out_iv);
}

bool ime_aead_dec(const uint32_t* key, const uint32_t* iv, const uint32_t* tag,
                  size_t len, const uint32_t* buf, uint32_t* out_buf)
{
    return ime_aead_dec_gen(key, iv, tag, len, PTR_32(buf), PTR_32(out_buf));
}

void ime_aead_enc_mram(const uint32_t* key, const uint32_t* iv,
    size_t len, const uint32_t __mram_ptr* buf, uint32_t __mram_ptr* out_buf,
    uint32_t* out_tag, uint32_t* out_iv)
{
    ime_aead_enc_gen(key, iv, len, PTR_32(buf), PTR_32(out_buf), out_tag, out_iv);
}

bool ime_aead_dec_mram(const uint32_t* key, const uint32_t* iv, const uint32_t* tag,
                       size_t len, const uint32_t __mram_ptr* buf, uint32_t __mram_ptr* out_buf)
{
    return ime_aead_dec_gen(key, iv, tag, len, PTR_32(buf), PTR_32(out_buf));
}
