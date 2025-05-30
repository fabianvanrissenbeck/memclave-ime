#include "global.h"

#include <stddef.h>

__attribute__((section(".data.persistent"))) void __mram_ptr* ime_sk_addr = NULL;
__attribute__((section(".data.persistent"))) uint32_t ime_sk_tag[4] = { 0 };
__attribute__((section(".data.persistent"))) uint32_t ime_user_key[8] = { 0 };
