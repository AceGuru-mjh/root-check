#ifndef CRYPTO_SHA3_H
#define CRYPTO_SHA3_H

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

#define SHA3_512_HASH_SIZE  64
#define SHA3_512_RATE       72   /* (1600 - 2*512) / 8 = 72 bytes */

typedef struct {
    uint64_t state[25];
    uint8_t  buf[SHA3_512_RATE];
    size_t   buf_len;
} sha3_512_ctx;

/*
 * Initialize SHA3-512 context.
 * Returns 0 on success, negative on error.
 */
CRYPTO_EXPORT int sha3_512_init(sha3_512_ctx *ctx);

/*
 * Feed data into SHA3-512 hash.
 * Returns 0 on success, negative on error.
 */
CRYPTO_EXPORT int sha3_512_update(sha3_512_ctx *ctx, const uint8_t *data, size_t len);

/*
 * Finalize hash and output 64-byte digest. Context is zeroed.
 * Returns 0 on success, negative on error.
 */
CRYPTO_EXPORT int sha3_512_final(sha3_512_ctx *ctx, uint8_t *hash);

/*
 * One-shot SHA3-512 hash.
 * Returns 0 on success, negative on error.
 */
CRYPTO_EXPORT int sha3_512_hash(const uint8_t *data, size_t len, uint8_t *hash);

#ifdef __cplusplus
}
#endif

#endif /* CRYPTO_SHA3_H */
