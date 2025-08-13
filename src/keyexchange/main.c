#include "mbedtls/dhm.h"

#include <alloc.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <perfcounter.h>

static mbedtls_dhm_context ctx;

const uint8_t ime_dhm_prime[] = MBEDTLS_DHM_RFC3526_MODP_2048_P_BIN;
const uint8_t ime_dhm_gen[] = MBEDTLS_DHM_RFC3526_MODP_2048_G_BIN;
const uint8_t ime_secret_key[32] = { 0 };

static struct {
    size_t len;
    uint64_t state[2048]; // 16 KiB
} g_alloc_state;

void mbedtls_zeroize_and_free(void* buf, size_t len) {}

void mbedtls_platform_zeroize(void* buf, size_t len) {}

void* calloc(size_t n, size_t sz) {
    size_t offset = (sz + 7) / 8;
    void* res = &g_alloc_state.state[g_alloc_state.len];

    if (g_alloc_state.len + offset < 2048) {
        g_alloc_state.len += offset;
        return res;
    } else {
        return NULL;
    }
}

int rng(void* ctx, uint8_t* buf, size_t len) { return 0; }

static uint8_t output[512] = { 0 };

int main(void) {
    mbedtls_mpi p;
    mbedtls_mpi g;
    mbedtls_mpi X;
    mbedtls_mpi x;

    mbedtls_mpi_init(&p);
    mbedtls_mpi_init(&g);
    mbedtls_mpi_init(&X);
    mbedtls_mpi_init(&x);

    assert(mbedtls_mpi_read_binary(&p, ime_dhm_prime, sizeof(ime_dhm_prime)) == 0);
    assert(mbedtls_mpi_read_binary(&g, ime_dhm_gen, sizeof(ime_dhm_gen)) == 0);
    assert(mbedtls_mpi_read_binary(&x, ime_secret_key, sizeof(ime_secret_key)) == 0);

    perfcounter_config(COUNT_CYCLES, true);
    assert(mbedtls_mpi_exp_mod(&X, &g, &x, &p, NULL) == 0);

    printf("mbedtls_mpi_exp_mode(): %llums\n", (uint64_t) perfcounter_get() * 1000 / CLOCKS_PER_SEC);
}