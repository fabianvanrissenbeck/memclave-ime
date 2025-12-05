#include "core.h"

#include <mram.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <mbedtls/md.h>
#include <mbedtls/hmac_drbg.h>
#include <mbedtls/chachapoly.h>

#include <mutex.h>

/**
 * Index that is unique per DPU used in one session.
 * Can be used to ensure, that e.g. each IV generated on a DPU
 * is different, simply by using the ID as the prefix for the global counter.
 */
__host uint32_t dpu_index;
/**
 * Information to seed the random number generator. DPUs don't have
 * access to good random numbers and therefore have to rely on a different
 * source.
 */
__host uint8_t key_seed[32];

__mram extern uint64_t g_sk_xchg_1[];
__mram extern uint64_t g_sk_xchg_2[];
__mram extern uint64_t g_sk_xchg_3[];
__mram extern uint64_t g_sk_msg[];
__mram extern uint64_t g_tl[];

mbedtls_hmac_drbg_context g_rnd;
uint8_t sys_key[32];

/**
 * Install whatever is in the global sys_key location, into
 * the registers of the key storage thread 22. Then also clear
 * the counter registers of the counter thread 23.
 */
extern void install_keys();

/**
 * Replace in memory with the TL. Loads TL code from g_tl.
 */
extern _Noreturn void replace();

static int nop(void* ctx, uint8_t* buf, size_t sz) {
    memset(buf, 0, sz);
    return 0;
}

void* calloc(size_t n, size_t sz) {
    // Called only twice by HMAC - Using the buddy alloc took an additional 2 KB of IRAM
    static uint64_t chunk[256];
    static size_t ptr = 0;

    assert(ptr < 2);
    return &chunk[(ptr++) * 128];
}

void free(void* ptr) {}

static void enc_sk(uint64_t __mram_ptr* sk_buf, uint8_t* key) {
    ime_sk __mram_ptr* sk = (ime_sk __mram_ptr*) sk_buf;
    mbedtls_chachapoly_context ctx;
    uint8_t iv[12];

    assert(mbedtls_hmac_drbg_random(&g_rnd, iv, sizeof(iv)) == 0);

    mbedtls_chachapoly_init(&ctx);
    mbedtls_chachapoly_setkey(&ctx, key);
    mbedtls_chachapoly_starts(&ctx, iv, MBEDTLS_CHACHAPOLY_ENCRYPT);

    /* feed the header portion as AAD into chachapoly */

    uint32_t start[8] = { 0xA5A5A5A5 };
    uint64_t header[4];

    mram_read(&sk_buf[4], &header[0], sizeof(header));

    assert(mbedtls_chachapoly_update_aad(&ctx, (const uint8_t*) &start[0], sizeof(start)) == 0);
    assert(mbedtls_chachapoly_update_aad(&ctx, (const uint8_t*) &header[0], sizeof(header)) == 0);

    /* start feeding aad parts of the subkernel */
    uint64_t buffer[2048 / sizeof(uint64_t)];

    for (size_t i = 64; i + sizeof(buffer) <= sk->size_aad; i += sizeof(buffer)) {
        size_t sz = sk->size_aad - i;
        sz = sz < sizeof(buffer) ? sz : sizeof(buffer);

        mram_read(&sk_buf[i / sizeof(uint64_t)], &buffer[0], sz);
        assert(mbedtls_chachapoly_update_aad(&ctx, (const uint8_t*) &buffer[0], sizeof(buffer)) == 0);
    }

    /* encrypt the leftover parts of the subkernel */

    for (size_t i = sk->size_aad; i + sizeof(buffer) <= sk->size; i += sizeof(buffer)) {
        size_t sz = sk->size - i;
        sz = sz < sizeof(buffer) ? sz : sizeof(buffer);

        mram_read(&sk_buf[i / sizeof(uint64_t)], &buffer[0], sz);
        assert(mbedtls_chachapoly_update(&ctx, sizeof(buffer), (const uint8_t*) &buffer[0], (uint8_t*) &buffer[0]) == 0);
    }

    uint8_t tag[16];

    assert(mbedtls_chachapoly_finish(&ctx, tag) == 0);
    mbedtls_chachapoly_free(&ctx);

    for (int i = 0; i < 4; ++i) {
        sk->tag[i] = ((const uint32_t*) tag)[i];
    }

    for (int i = 0; i < 3; ++i) {
        sk->iv[i] = ((const uint32_t*) iv)[i];
    }
}

int main(void) {
    const char* custom = "ime-first-stage-loader";
    mbedtls_hmac_drbg_init(&g_rnd);

    assert(mbedtls_hmac_drbg_seed(&g_rnd, mbedtls_md_info_from_type(MBEDTLS_MD_MD5), nop, NULL, (const uint8_t*) custom, 22) == 0);
    assert(mbedtls_hmac_drbg_update(&g_rnd, key_seed, sizeof(key_seed)) == 0);

    assert(mbedtls_hmac_drbg_random(&g_rnd, sys_key, sizeof(sys_key)) == 0);

    enc_sk(g_sk_xchg_1, sys_key);
    enc_sk(g_sk_xchg_2, sys_key);
    enc_sk(g_sk_xchg_3, sys_key);
    enc_sk(g_sk_msg, sys_key);

    // use dpu_index to pass r0 value to counter thread
    dpu_index = ((dpu_index << 1) & 0xFFFE) | 1;

    // because replace is in a section called .replace and not .text
    // ld removes all uses of g_tl, preventing its address from being visible.
    asm volatile(
        "jump .+2\n"
        "move r0, g_tl\n"
    );

    install_keys();
    replace();
}
