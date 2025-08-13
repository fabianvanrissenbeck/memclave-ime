#include <ime.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

extern uint64_t __mram_noinit __ime_msg_sk[];

static const uint32_t key_template[8] = {
    0x83828180,
    0x87868584,
    0x8b8a8988,
    0x8f8e8d8c,
    0x93929190,
    0x97969594,
    0x9b9a9998,
    0x9f9e9d9c,
};

__attribute__((section(".data.persist")))
static uint32_t key[8];

int main(void) {
    uint32_t* ptr = (uint32_t*) (32 << 10);
    assert(ptr[0] == 0x0);
    ptr[0] = 0x12345678;

    memcpy(key, key_template, sizeof(key_template));
    __ime_replace_sk(__ime_msg_sk, NULL, key);
}
