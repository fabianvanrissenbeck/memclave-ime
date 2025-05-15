#include <perfcounter.h>
#include <mbedtls/chachapoly.h>

const uint8_t key[32];
const uint8_t iv[12];
uint8_t data[8096];
uint8_t tag[16];

void mbedtls_platform_zeroize(void *buf, size_t len) {
    volatile uint8_t* data = (volatile void*) buf;

    for (size_t i = 0; i < len; i++) {
        data[i] = 0;
    }
}

int main(void) {
    mbedtls_chachapoly_context ctx;

    mbedtls_chachapoly_init(&ctx);
    mbedtls_chachapoly_setkey(&ctx, key);

    perfcounter_config(COUNT_CYCLES, true);
    mbedtls_chachapoly_encrypt_and_tag(&ctx, sizeof(data), iv, NULL, 0, data, data, tag);

    perfcounter_t count = perfcounter_get();
    return count & 0xF;
}