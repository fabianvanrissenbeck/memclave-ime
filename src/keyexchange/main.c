#include "mbedtls/ecdh.h"

#include <alloc.h>
#include <assert.h>
#include <string.h>

void* calloc(size_t n, size_t sz) { return NULL; }
void free(void* ptr) {}

int main(void) {
#if 0
    mbedtls_ecdh_context ctx;
    mbedtls_ecdh_init(&ctx);
#endif


#if 0
    mbedtls_ecdh_gen_public(NULL, NULL, NULL, NULL, NULL);
#endif
    mbedtls_ecdh_compute_shared(NULL, NULL, NULL, NULL, NULL, NULL);
}