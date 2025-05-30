#ifndef GLOBAL_H
#define GLOBAL_H

#include <mram.h>
#include <stdint.h>

__attribute__((section(".data.persistent"))) extern void __mram_ptr* ime_sk_addr;
__attribute__((section(".data.persistent"))) extern uint32_t ime_sk_tag[4];
__attribute__((section(".data.persistent"))) extern uint32_t ime_user_key[8];

#endif
