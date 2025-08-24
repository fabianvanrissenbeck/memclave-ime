#include "ime.h"
#include "common.h"
#include "mbedtls/sha256.h"

__attribute__((section(".data.persist")))
struct ime_xchg_io g_xchg_io;

int main(void) {
    mbedtls_sha256_context ctx;
    volatile uint32_t key[8];

    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, (const uint8_t*) g_xchg_io.out.xchg_shared_out, sizeof(g_xchg_io.out.xchg_shared_out));
    mbedtls_sha256_update(&ctx, (const uint8_t*) g_xchg_io.out.xchg_cnt_out, sizeof(g_xchg_io.out.xchg_cnt_out));
    mbedtls_sha256_finish(&ctx, (uint8_t*) key);

    for (int i = 0; i < 8; ++i) {
        __ime_debug_out[i] = key[i];
    }

    for (int i = 0; i < 4; ++i) {
        __ime_debug_out[i + 8] = g_xchg_io.out.xchg_cnt_out[i];
    }

    for (int i = 0; i < 4; ++i) {
        __ime_debug_out[i + 12] = g_xchg_io.out.xchg_shared_out[i];
    }
}