#ifndef BLAKE2_H
#define BLAKE2_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BLAKE2B_OUTBYTES 64
#define BLAKE2B_BLOCKBYTES 128
#define BLAKE2S_OUTBYTES 32
#define BLAKE2S_BLOCKBYTES 64

typedef struct {
    uint64_t h[8];
    uint64_t t[2];
    uint64_t f[2];
    uint8_t  buf[256];
    size_t   buflen;
    size_t   outlen;
} blake2b_ctx;

typedef struct {
    uint32_t h[8];
    uint32_t t[2];
    uint32_t f[2];
    uint8_t  buf[128];
    size_t   buflen;
    size_t   outlen;
} blake2s_ctx;

void blake2b_init(blake2b_ctx *ctx, size_t outlen);
void blake2b_update(blake2b_ctx *ctx, const uint8_t *data, size_t len);
void blake2b_final(blake2b_ctx *ctx, uint8_t *out);
void blake2b(const uint8_t *data, size_t len, uint8_t *out, size_t outlen);

void blake2s_init(blake2s_ctx *ctx, size_t outlen);
void blake2s_update(blake2s_ctx *ctx, const uint8_t *data, size_t len);
void blake2s_final(blake2s_ctx *ctx, uint8_t *out);
void blake2s(const uint8_t *data, size_t len, uint8_t *out, size_t outlen);

#ifdef __cplusplus
}
#endif

#endif
