#include "ime.h"
#include "common.h"

#include "mbedtls/dhm.h"
#include "mbedtls/bignum.h"

#include <alloc.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <perfcounter.h>

// little endian version of MBEDTLS_DHM_RFC3526_MODP_2048_P_BIN
const uint32_t ime_raw_prime[] = {
    0xffffffff, 0xffffffff, 0x8aacaa68, 0x15728e5a,
    0x98fa0510, 0x15d22618, 0xea956ae5, 0x3995497c,
    0x95581718, 0xde2bcbf6, 0x6f4c52c9, 0xb5c55df0,
    0xec07a28f, 0x9b2783a2, 0x180e8603, 0xe39e772c,
    0x2e36ce3b, 0x32905e46, 0xca18217c, 0xf1746c08,
    0x4abc9804, 0x670c354e, 0x7096966d, 0x9ed52907,
    0x208552bb, 0x1c62f356, 0xdca3ad96, 0x83655d23,
    0xfd24cf5f, 0x69163fa8, 0x1c55d39a, 0x98da4836,
    0xa163bf05, 0xc2007cb8, 0xece45b3d, 0x49286651,
    0x7c4b1fe6, 0xae9f2411, 0x5a899fa5, 0xee386bfb,
    0xf406b7ed, 0xbff5cb6, 0xa637ed6b, 0xf44c42e9,
    0x625e7ec6, 0xe485b576, 0x6d51c245, 0x4fe1356d,
    0xf25f1437, 0x302b0a6d, 0xcd3a431b, 0xef9519b3,
    0x8e3404dd, 0x514a0879, 0x3b139b22, 0x20bbea6,
    0x8a67cc74, 0x29024e08, 0x80dc1cd1, 0xc4c6628b,
    0x2168c234, 0xc90fdaa2, 0xffffffff, 0xffffffff
};

const uint32_t ime_raw_sk[] = {
    0x69c7e189, 0xd0997133, 0xf1623124, 0xc7169218,
    0xbad0d374, 0x6b8e7824, 0x83338e41, 0x00db6a03,
};

const mbedtls_mpi ime_dhm_p = {
    .private_p = (uint32_t*) ime_raw_prime,
    .private_n = sizeof(ime_raw_prime) / sizeof(uint32_t),
    .private_s = 1
};

const mbedtls_mpi ime_dhm_s = {
    .private_p = (uint32_t*) ime_raw_sk,
    .private_n = sizeof(ime_raw_sk) / sizeof(uint32_t),
    .private_s = 1
};

__attribute__((aligned(4)))
const uint8_t ime_dhm_gen[] = MBEDTLS_DHM_RFC3526_MODP_2048_G_BIN;

size_t n_alloc = 0;
size_t m_alloc = 0;

__attribute__((section(".data.persist")))
struct ime_xchg_io g_xchg_io;

const mbedtls_mpi ime_client_pub = {
    .private_p = g_xchg_io.in.client_pk_in,
    .private_n = sizeof(g_xchg_io.in.client_pk_in) / sizeof(uint32_t),
    .private_s = 1
};

void* calloc(size_t n, size_t sz) {
    // alloc_db[n_alloc++] = n * sz;
    n_alloc += 1;
    m_alloc += n * sz;
    return buddy_alloc(n * sz);
}

void free(void* p) {
    buddy_free(p);
}

int main(void) {
    mbedtls_mpi pk;

    buddy_init(8192 * 2);
    mbedtls_mpi_init(&pk);

    int res = mbedtls_mpi_exp_mod(&pk, &ime_client_pub, &ime_dhm_s, &ime_dhm_p, NULL);

    if (res == MBEDTLS_ERR_MPI_ALLOC_FAILED) {
        printf("Allocation Failed\n");
    }

    assert(res == 0);

    for (int i = 0; i < 64; ++i) {
        g_xchg_io.out.xchg_shared_out[i] = pk.private_p[i];
    }

    __ime_replace_sk(__ime_xchg_sk_3, NULL, NULL);
    __builtin_unreachable();
}
