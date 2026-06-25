#ifndef CRYPTO_AES_GCM_H
#define CRYPTO_AES_GCM_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CRYPTO_EXPORT
#ifdef __GNUC__
#define CRYPTO_EXPORT __attribute__((visibility("default")))
#else
#define CRYPTO_EXPORT
#endif
#endif

#define AES_256_KEY_SIZE    32
#define AES_BLOCK_SIZE      16
#define GCM_IV_SIZE         12
#define GCM_TAG_SIZE        16

typedef struct {
    uint32_t rk[60];        /* Round keys for AES-256 (14 rounds + 1) */
    uint8_t  H[16];         /* Hash key for GHASH */
    uint8_t  J0[16];        /* Initial counter block */
    uint8_t  ctr[16];       /* Current counter */
    uint8_t  ghash[16];     /* GHASH accumulator */
    uint64_t aad_len;       /* AAD length in bits */
    uint64_t ct_len;        /* Ciphertext length in bits */
} aes_gcm_ctx;

/*
 * Initialize AES-256-GCM context with key and IV.
 * key: 32-byte key
 * iv: 12-byte IV (recommended)
 * Returns 0 on success, negative on error.
 */
CRYPTO_EXPORT int aes_gcm_init(aes_gcm_ctx *ctx, const uint8_t *key, const uint8_t *iv, size_t iv_len);

/*
 * Add associated data (AAD) for authentication.
 * Can be called multiple times before encryption/decryption.
 * Returns 0 on success, negative on error.
 */
CRYPTO_EXPORT int aes_gcm_update_aad(aes_gcm_ctx *ctx, const uint8_t *aad, size_t len);

/*
 * Encrypt plaintext and update authentication tag.
 * Can be called multiple times for streaming encryption.
 * out: output buffer (same size as len)
 * Returns 0 on success, negative on error.
 */
CRYPTO_EXPORT int aes_gcm_update_encrypt(aes_gcm_ctx *ctx, uint8_t *out, const uint8_t *in, size_t len);

/*
 * Decrypt ciphertext and update authentication tag.
 * Can be called multiple times for streaming decryption.
 * out: output buffer (same size as len)
 * Returns 0 on success, negative on error.
 */
CRYPTO_EXPORT int aes_gcm_update_decrypt(aes_gcm_ctx *ctx, uint8_t *out, const uint8_t *in, size_t len);

/*
 * Finalize encryption and output authentication tag.
 * tag: output buffer for 16-byte tag
 * Returns 0 on success, negative on error.
 */
CRYPTO_EXPORT int aes_gcm_final_encrypt(aes_gcm_ctx *ctx, uint8_t *tag);

/*
 * Finalize decryption and verify authentication tag.
 * tag: 16-byte tag to verify
 * Returns 0 on success (tag valid), negative on error or tag mismatch.
 */
CRYPTO_EXPORT int aes_gcm_final_decrypt(aes_gcm_ctx *ctx, const uint8_t *tag);

/*
 * One-shot AES-256-GCM encryption.
 * key: 32-byte key
 * iv: 12-byte IV
 * aad: associated data (can be NULL if aad_len == 0)
 * aad_len: length of AAD
 * plaintext: input data
 * pt_len: length of plaintext
 * ciphertext: output buffer (pt_len bytes)
 * tag: output buffer for 16-byte authentication tag
 * Returns 0 on success, negative on error.
 */
CRYPTO_EXPORT int aes_gcm_encrypt(
    const uint8_t *key,
    const uint8_t *iv,
    const uint8_t *aad, size_t aad_len,
    const uint8_t *plaintext, size_t pt_len,
    uint8_t *ciphertext,
    uint8_t *tag
);

/*
 * One-shot AES-256-GCM decryption.
 * key: 32-byte key
 * iv: 12-byte IV
 * aad: associated data (can be NULL if aad_len == 0)
 * aad_len: length of AAD
 * ciphertext: input data
 * ct_len: length of ciphertext
 * plaintext: output buffer (ct_len bytes)
 * tag: 16-byte authentication tag to verify
 * Returns 0 on success (tag valid), negative on error or tag mismatch.
 */
CRYPTO_EXPORT int aes_gcm_decrypt(
    const uint8_t *key,
    const uint8_t *iv,
    const uint8_t *aad, size_t aad_len,
    const uint8_t *ciphertext, size_t ct_len,
    uint8_t *plaintext,
    const uint8_t *tag
);

#ifdef __cplusplus
}
#endif

#endif /* CRYPTO_AES_GCM_H */
