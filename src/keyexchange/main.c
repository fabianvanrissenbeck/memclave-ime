#include "mbedtls/dhm.h"

#include <alloc.h>
#include <assert.h>
#include <string.h>

static mbedtls_dhm_context ctx;

const uint8_t ime_dhm_prime[] = MBEDTLS_DHM_RFC3526_MODP_2048_P_BIN;
const uint8_t ime_dhm_gen[] = MBEDTLS_DHM_RFC3526_MODP_2048_G_BIN;

void mbedtls_zeroize_and_free(void* buf, size_t len) {}

void mbedtls_platform_zeroize(void* buf, size_t len) {}

void* calloc(size_t n, size_t sz) { return NULL; }

int rng(void* ctx, uint8_t* buf, size_t len) { return 0; }

static uint8_t output[512] = { 0 };

int main(void) {
    mbedtls_mpi p;
    mbedtls_mpi g;

#if 0
    mbedtls_mpi_init(&p);
    mbedtls_mpi_init(&g);

    mbedtls_mpi_read_binary(&p, ime_dhm_prime, sizeof(ime_dhm_prime));
    mbedtls_mpi_read_binary(&g, ime_dhm_gen, sizeof(ime_dhm_gen));

    mbedtls_dhm_init(&ctx);
    assert(mbedtls_dhm_set_group(&ctx, &p, &g) == 0);
    assert(mbedtls_dhm_make_public(&ctx, 32, output, 512, rng, NULL) == 0);

#else
    mbedtls_dhm_calc_secret(&ctx, NULL, NULL, NULL, NULL, NULL);
#endif

    // mbedtls_mpi_exp_mod(NULL, NULL, NULL, NULL, NULL);

    // dhm_set_group
    // dhm_make_public
    asm("stop");
}