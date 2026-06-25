#ifndef XCHACHA20_H
#define XCHACHA20_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XCHACHA20_KEY_SIZE 32
#define XCHACHA20_NONCE_SIZE 24
#define XCHACHA20_BLOCK_SIZE 64

typedef struct {
    uint32_t state[16];
    uint8_t  key[XCHACHA20_KEY_SIZE];
    uint8_t  nonce[XCHACHA20_NONCE_SIZE];
    uint32_t counter;
} xchacha20_ctx;

void xchacha20_init(xchacha20_ctx *ctx,
                    const uint8_t key[32],
                    const uint8_t nonce[24]);
void xchacha20_encrypt(xchacha20_ctx *ctx,
                       const uint8_t *plain, uint8_t *cipher, size_t len);
void xchacha20_decrypt(xchacha20_ctx *ctx,
                       const uint8_t *cipher, uint8_t *plain, size_t len);

#ifdef __cplusplus
}
#endif

#endif
