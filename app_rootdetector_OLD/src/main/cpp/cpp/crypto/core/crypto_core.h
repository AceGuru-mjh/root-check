#ifndef CRYPTO_CORE_H
#define CRYPTO_CORE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== SHA3-256 ========== */
#define SHA3_256_DIGEST_LEN 32
#define SHA3_256_RATE 136

/* ========== SHA3-512（可信密码学验签核心）========== */
#define SHA3_512_DIGEST_LEN 64
#define SHA3_512_RATE 72

typedef struct {
    uint64_t state[25];
    size_t rate;
    size_t capacity;
    size_t output_len;
    uint8_t buf[144];
    size_t buf_len;
    size_t block_size;
} sha3_ctx;

void sha3_256_init(sha3_ctx *ctx);
void sha3_256_update(sha3_ctx *ctx, const uint8_t *data, size_t len);
void sha3_256_final(sha3_ctx *ctx, uint8_t *digest);
void sha3_256(const uint8_t *data, size_t len, uint8_t *digest);

/* SHA3-512 API（新增，全域可信验签） */
void sha3_512_init(sha3_ctx *ctx);
void sha3_512_update(sha3_ctx *ctx, const uint8_t *data, size_t len);
void sha3_512_final(sha3_ctx *ctx, uint8_t *digest);
void sha3_512(const uint8_t *data, size_t len, uint8_t *digest);

/* ========== AES-256-GCM ========== */
#define AES_GCM_KEY_SIZE 32
#define AES_GCM_IV_SIZE 12
#define AES_GCM_TAG_SIZE 16
#define AES_GCM_BLOCK_SIZE 16

typedef struct {
    uint32_t rk[64];
    int rounds;
} aes256_ctx;

void aes256_init(aes256_ctx *ctx, const uint8_t *key);
void aes256_encrypt_block(aes256_ctx *ctx, const uint8_t *in, uint8_t *out);
int aes256_gcm_encrypt(const uint8_t *plain, size_t plain_len,
                       const uint8_t *key, const uint8_t *iv,
                       uint8_t *cipher, uint8_t *tag);
int aes256_gcm_decrypt(const uint8_t *cipher, size_t cipher_len,
                       const uint8_t *key, const uint8_t *iv,
                       const uint8_t *tag, uint8_t *plain);

/* ========== 简易加密工具 ========== */
void xor_data(uint8_t *data, size_t len, const uint8_t *key, size_t key_len);
uint32_t crc32_simple(const uint8_t *data, size_t len);
uint64_t hash64(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* CRYPTO_CORE_H */