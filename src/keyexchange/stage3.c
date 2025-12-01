#include "ime.h"
#include "aead.h"
#include "common.h"
#include "mbedtls/sha256.h"

__attribute__((section(".data.persist")))
struct ime_xchg_io g_xchg_io;

__mram uint32_t g_xchg_out[8];
__mram uint32_t g_output_tag[4];
__mram uint32_t g_output_iv[3];

// NOTE: The attribute((aligned(8))) is STRICTLY necessary here.
// Clang replaces the for loops with WRAM->MRAM memcpy calls. These
// expect 8-byte aligned WRAM buffers, BUT uint32_t arrays are not
// strictly 8-byte aligned.

int main(void) {
    mbedtls_sha256_context ctx;
    __attribute__((aligned(8))) uint32_t key[8];

    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, (const uint8_t*) g_xchg_io.out.xchg_shared_out, sizeof(g_xchg_io.out.xchg_shared_out));
    mbedtls_sha256_update(&ctx, (const uint8_t*) g_xchg_io.out.xchg_cnt_out, sizeof(g_xchg_io.out.xchg_cnt_out));
    mbedtls_sha256_finish(&ctx, (uint8_t*) key);

    __attribute__((aligned(8))) uint32_t final_key[8];

    if (!ime_aead_dec(key, (uint32_t[3]) { 0 }, &g_xchg_io.out.key_enc_out[8], 32, g_xchg_io.out.key_enc_out, final_key)) {
        // exiting main this way will clear all data in WRAM (including key material)
        // after this the MSG subkernel will be invoked, allowing a new key exchange to start
        return 1;
    }

    for (int i = 0; i < 8; ++i) {
        g_load_prop.key[i] = final_key[i];
    }

    __attribute__((aligned(8))) const uint8_t confirm[] = "Exchange Succeeded Successfully!";
    mram_write(confirm, g_xchg_out, sizeof(confirm) - 1);

    uint32_t confirm_iv[3];
    uint32_t confirm_tag[4];

    ime_aead_enc_mram(final_key, NULL, sizeof(confirm) - 1, g_xchg_out, g_xchg_out, confirm_tag, confirm_iv);

    for (int i = 0; i < 4; ++i) {
        g_output_tag[i] = confirm_tag[i];
    }

    for (int i = 0; i < 3; ++i) {
        g_output_iv[i] = confirm_iv[i];
    }

    __ime_replace_sk(__ime_msg_sk, NULL, NULL);
    __builtin_unreachable();
}
