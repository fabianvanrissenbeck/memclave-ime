#include <mbedtls/sha256.h>

unsigned char test_string[] = "Hallo, Welt!";

void mbedtls_platform_zeroize(void* buf, size_t len) {
    volatile uint8_t* mem = (volatile void*) buf;

    for (size_t i = 0; i < len; i++) {
        mem[i] = 0;
    }
}

int main(void) {
    uint8_t hash[32];

    mbedtls_sha256_context sha256;
    mbedtls_sha256_init(&sha256);
    mbedtls_sha256_starts(&sha256, 0);
    mbedtls_sha256_update(&sha256, test_string, sizeof(test_string) - 1);
    mbedtls_sha256_finish(&sha256, hash);

    return 0;
}