#include "aead.h"
#include "poly.h"
#include "ime.h"

void ime_aead_enc(const uint32_t* key, const uint32_t* p_iv,
                  size_t len, const uint32_t* buf, uint32_t* out_buf,
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

    poly_context ctx;
    uint32_t block[16];

    __ime_chacha_blk(key, 0, iv[0], iv[1], iv[2], block);
    poly_init(&ctx, block);

    for (size_t i = 0; i < (len + 63) / 64; ++i) {
        __ime_chacha_blk(key, i + 1, iv[0], iv[1], iv[2], block);

        for (size_t j = 0; j < 16 && i * 16 + j < len / 4; ++j) {
            out_buf[i * 16 + j] = buf[i * 16 + j] ^ block[j];
        }
    }

    for (size_t i = 0; i < len / 16; ++i) {
        poly_feed_block(&ctx, &out_buf[i * 4]);
    }

    poly_feed_block(&ctx, (uint32_t[4]) { 0, 0, len, 0 });
    poly_finalize(&ctx, out_tag);

    if (out_iv) {
        out_iv[0] = iv[0];
        out_iv[1] = iv[1];
        out_iv[2] = iv[2];
    }
}

bool ime_aead_dec(const uint32_t* key, const uint32_t* iv, const uint32_t* tag,
                  size_t len, const uint32_t* buf, uint32_t* out_buf)
{
    poly_context ctx;
    uint32_t block[16];
    bool valid = true;

    __ime_chacha_blk(key, 0, iv[0], iv[1], iv[2], block);
    poly_init(&ctx, block);

    for (size_t i = 0; i < len / 16; ++i) {
        poly_feed_block(&ctx, &buf[i * 4]);
    }

    poly_feed_block(&ctx, (uint32_t[4]){ 0, 0, len, 0 });
    poly_finalize(&ctx, block);

    for (size_t i = 0; i < 4; ++i) {
        valid &= block[i] == tag[i];
    }

    if (!valid) { return false; }

    for (size_t i = 0; i < (len + 63) / 64; ++i) {
        __ime_chacha_blk(key, i + 1, iv[0], iv[1], iv[2], block);

        for (size_t j = 0; j < 16 && i * 16 + j < len / 4; ++j) {
            out_buf[i * 16 + j] = buf[i * 16 + j] ^ block[j];
        }
    }

    return true;
}