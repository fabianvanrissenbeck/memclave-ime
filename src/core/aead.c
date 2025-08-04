#include "aead.h"
#include "poly.h"

#include <assert.h>

#define OUTPUT ((uint32_t __mram_ptr*) ((64 << 20) - 64))

extern void ime_chacha_blk(uint32_t key[8], uint32_t c, uint32_t iv_a, uint32_t iv_b, uint32_t iv_c);

extern uint32_t chacha_output[16];

static void fetch_sk_chunk(const ime_sk __mram_ptr* sk, size_t chunk, uint32_t out[4]) {
    const uint32_t __mram_ptr* sk_raw = (const uint32_t __mram_ptr*) sk;

    for (int i = 0; i < 4; ++i) {
        out[i] = sk_raw[chunk * 4 + i];
    }
}

static void xor_sk_chunk(const ime_sk __mram_ptr* sk, size_t chunk, const uint32_t data[16]) {
    uint32_t __mram_ptr* sk_raw = (uint32_t __mram_ptr*) sk;

    for (int i = 0; i < 16; ++i) {
        sk_raw[chunk * 16 + i] ^= data[i];
    }
}

bool ime_decrypt_verify(ime_sk __mram_ptr* sk) {
    poly_context ctx;
    uint32_t tag[4];

    uint32_t key[8] = {
        0x83828180,
        0x87868584,
        0x8b8a8988,
        0x8f8e8d8c,
        0x93929190,
        0x97969594,
        0x9b9a9998,
        0x9f9e9d9c,
    };

    ime_chacha_blk(key, 0, sk->iv[0], sk->iv[1], sk->iv[2]);
    poly_init(&ctx, chacha_output);

    // feed header and AEAD portion of the subkernel

    poly_feed_block(&ctx, (uint32_t[4]) { 0xA5A5A5A5 });
    poly_feed_block(&ctx, (uint32_t[4]) { 0 });

    for (size_t i = 2; i < sk->size_aad / 16; ++i) {
        uint32_t buf[4];

        fetch_sk_chunk(sk, i, buf);
        poly_feed_block(&ctx, buf);

        assert(i != 3 || (buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 0));
    }

    // subkernel components are 16 byte aligned, no padding is required
    // feed ciphertext portion of the subkernel

    for (size_t i = sk->size_aad / 16; i < sk->size / 16; ++i) {
        uint32_t buf[4];

        fetch_sk_chunk(sk, i, buf);
        poly_feed_block(&ctx, buf);
    }

    // feed length to finalize aead computation
    poly_feed_block(&ctx, (uint32_t[4]) { sk->size_aad, 0x0, sk->size - sk->size_aad, 0x0 });
    poly_finalize(&ctx, tag);

#if 0
    for (int i = 0; i < 4; ++i) {
        OUTPUT[i] = tag[i];
        OUTPUT[i + 4] = sk->tag[i];
    }
#endif

#if 0
    for (int i = 0; i < 4; ++i) {
        if (tag[i] != sk->tag[i]) {
            return false;
        }
    }
#endif

    for (size_t i = sk->size_aad / 64; i < sk->size / 64; ++i) {
        ime_chacha_blk(key, i - sk->size_aad / 64 + 1, sk->iv[0], sk->iv[1], sk->iv[2]);
        xor_sk_chunk(sk, i, chacha_output);
    }

    return true;
}
