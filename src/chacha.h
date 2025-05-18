#ifndef CHACHA_H
#define CHACHA_H

#include <stdint.h>

/**
 * @brief perform the chacha block function using a built-in key
 * @param iv 12-byte initialization vector
 * @param count 4-byte counter
 * @param out 64-byte location to write resulting block to
 */
void chacha_block_at(const uint8_t* iv, uint32_t count, uint8_t* out);

#endif
