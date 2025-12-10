#ifndef AEAD_H
#define AEAD_H

#include <mram.h>
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

void ime_aead_enc_mram(const uint32_t* key, const uint32_t* iv,
    size_t len, const uint32_t __mram_ptr* buf, uint32_t __mram_ptr* out_buf,
    uint32_t* out_tag, uint32_t* out_iv);

bool ime_aead_dec_mram(const uint32_t* key, const uint32_t* iv, const uint32_t* tag,
                       size_t len, const uint32_t __mram_ptr* buf, uint32_t __mram_ptr* out_buf);

/**
 * @brief Encrypt a buffer using the ChaCha20 stream cipher
 *
 * Warning: ChaCha20 does not ensure authenticity of contents.
 *
 * @param key key used for encryption
 * @param iv iv used for encryption - NULL to use global counter
 * @param len length of the buffer to encrypt - must be a multiple of 16
 * @param buf buffer to encrypt
 * @param out_buf buffer to write result to - may be the same as buf
 * @param out_iv location to write used iv to - may be NULL
 */
void ime_chacha_enc(const uint32_t* key, const uint32_t* iv,
                    size_t len, const uint32_t* buf,
                    uint32_t* out_buf, uint32_t* out_iv);

/**
 * Decrypt a buffer encrypted using ChaCha20
 *
 * Warning: ChaCha20 does not ensure authenticity of contents.
 *
 * @param key key used for decryption
 * @param iv iv used for decryption
 * @param len length of the buffer to decrypt - must be a multiple of 16
 * @param buf buffer to decrypt
 * @param out_buf buffer to write results to - may be the same as buf
 * @return true if decryption succeeded; false otherwise
 */
void ime_chacha_dec(const uint32_t* key, const uint32_t* iv,
                    size_t len, const uint32_t* buf, uint32_t* out_buf);


/**
 * @brief Performs the same action as the non-MRAM variant, except for the stride parameter.
 * @param stride Continue stride 64-byte blocks after the current block (with the correct counter value).
 *               This allows multiple threads to decrypt in parallel.
 */
void ime_chacha_enc_mram(const uint32_t* key, const uint32_t* iv, uint32_t stride,
                         size_t len, const uint32_t __mram_ptr* buf,
                         uint32_t __mram_ptr* out_buf, uint32_t* out_iv);

/**
 * @brief Performs the same action as the non-MRAM variant, except for the stride parameter.
 * @param stride see ime_chacha_enc_mram
 */
void ime_chacha_dec_mram(const uint32_t* key, const uint32_t* iv, uint32_t stride,
                         size_t len, const uint32_t __mram_ptr* buf, uint32_t __mram_ptr* out_buf);


#endif
