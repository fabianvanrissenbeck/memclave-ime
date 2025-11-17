#ifndef AEAD_H
#define AEAD_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * Encrypt a buffer using ChaCha20-Poly1305.
 * @param key key used for encryption
 * @param iv iv used for encryption - NULL to use global counter
 * @param len length of the buffer to encrypt - must be a multiple of 16
 * @param buf buffer to encrypt
 * @param out_buf buffer to write result to - may be the same as buf
 * @param out_tag location to write tag to
 * @param out_iv location to write used iv to - may be NULL
 */
void ime_aead_enc(const uint32_t* key, const uint32_t* iv,
                  size_t len, const uint32_t* buf, uint32_t* out_buf,
                  uint32_t* out_tag, uint32_t* out_iv);

/**
 * Decrypt a buffer encrypted using ChaCha20-Poly1305
 * @param key key used for decryption
 * @param iv iv used for decryption
 * @param tag tag for verifying integrity
 * @param len length of the buffer to decrypt - must be a multiple of 16
 * @param buf buffer to decrypt
 * @param out_buf buffer to write results to - may be the same as buf
 * @return true if decryption succeeded; false otherwise
 */
bool ime_aead_dec(const uint32_t* key, const uint32_t* iv, const uint32_t* tag,
                  size_t len, const uint32_t* buf, uint32_t* out_buf);

#endif
