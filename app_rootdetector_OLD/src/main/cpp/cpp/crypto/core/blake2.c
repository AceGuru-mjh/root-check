#include "blake2.h"
#include <string.h>

#define BLAKE2B_IV64_0 0x6a09e667f3bcc908ULL
#define BLAKE2B_IV64_1 0xbb67ae8584caa73bULL
#define BLAKE2B_IV64_2 0x3c6ef372fe94f82bULL
#define BLAKE2B_IV64_3 0xa54ff53a5f1d36f1ULL
#define BLAKE2B_IV64_4 0x510e527fade682d1ULL
#define BLAKE2B_IV64_5 0x9b05688c2b3e6c1fULL
#define BLAKE2B_IV64_6 0x1f83d9abfb41bd6bULL
#define BLAKE2B_IV64_7 0x5be0cd19137e2179ULL

static const uint64_t blake2b_iv[8] = {
    BLAKE2B_IV64_0, BLAKE2B_IV64_1, BLAKE2B_IV64_2, BLAKE2B_IV64_3,
    BLAKE2B_IV64_4, BLAKE2B_IV64_5, BLAKE2B_IV64_6, BLAKE2B_IV64_7
};

static const uint32_t blake2s_iv[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

static uint64_t rotr64(uint64_t x, uint32_t n) {
    return (x >> n) | (x << (64 - n));
}

static uint32_t rotr32(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

static void blake2s_compress(blake2s_ctx *ctx) {
    uint32_t m[16], v[16];
    for (int i = 0; i < 8; i++) v[i] = ctx->h[i];
    for (int i = 0; i < 8; i++) v[i+8] = blake2s_iv[i];
    v[12] ^= ctx->t[0];
    v[13] ^= ctx->t[1];
    v[14] ^= ctx->f[0];
    v[15] ^= ctx->f[1];
    for (int i = 0; i < 16; i++) m[i] = ((uint32_t *)ctx->buf)[i];
    for (int r = 0; r < 10; r++) {
        v[0] += v[4] + m[0]; v[12] = rotr32(v[12] ^ v[0], 16);
        v[8] += v[12]; v[4] = rotr32(v[4] ^ v[8], 12);
        v[0] += v[4] + m[1]; v[12] = rotr32(v[12] ^ v[0], 8);
        v[8] += v[12]; v[4] = rotr32(v[4] ^ v[8], 7);
        v[2] += v[6] + m[2]; v[14] = rotr32(v[14] ^ v[2], 16);
        v[10] += v[14]; v[6] = rotr32(v[6] ^ v[10], 12);
        v[2] += v[6] + m[3]; v[14] = rotr32(v[14] ^ v[2], 8);
        v[10] += v[14]; v[6] = rotr32(v[6] ^ v[10], 7);
    }
    for (int i = 0; i < 8; i++) ctx->h[i] ^= v[i] ^ v[i+8];
}

static void blake2b_compress(blake2b_ctx *ctx) {
    uint64_t m[16];
    uint64_t v[16];

    for (int i = 0; i < 8; i++) v[i] = ctx->h[i];
    for (int i = 0; i < 8; i++) v[i+8] = blake2b_iv[i];
    v[12] ^= ctx->t[0];
    v[13] ^= ctx->t[1];
    v[14] ^= ctx->f[0];
    v[15] ^= ctx->f[1];

    for (int i = 0; i < 16; i++) {
        m[i] = ((uint64_t *)ctx->buf)[i];
    }

    for (int r = 0; r < 12; r++) {
        v[0] += v[4] + m[0]; v[12] = rotr64(v[12] ^ v[0], 32);
        v[8] += v[12]; v[4] = rotr64(v[4] ^ v[8], 24);
        v[0] += v[4] + m[1]; v[12] = rotr64(v[12] ^ v[0], 16);
        v[8] += v[12]; v[4] = rotr64(v[4] ^ v[8], 63);
        v[2] += v[6] + m[2]; v[14] = rotr64(v[14] ^ v[2], 32);
        v[10] += v[14]; v[6] = rotr64(v[6] ^ v[10], 24);
        v[2] += v[6] + m[3]; v[14] = rotr64(v[14] ^ v[2], 16);
        v[10] += v[14]; v[6] = rotr64(v[6] ^ v[10], 63);
    }

    for (int i = 0; i < 8; i++) ctx->h[i] ^= v[i] ^ v[i+8];
}

void blake2b_init(blake2b_ctx *ctx, size_t outlen) {
    memset(ctx, 0, sizeof(blake2b_ctx));
    for (int i = 0; i < 8; i++) ctx->h[i] = blake2b_iv[i];
    ctx->h[0] ^= 0x01010000 ^ (uint64_t)outlen;
    ctx->outlen = outlen;
}

void blake2b_update(blake2b_ctx *ctx, const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        ctx->buf[ctx->buflen++] = data[i];
        if (ctx->buflen == 128) {
            ctx->t[0] += ctx->buflen;
            if (ctx->t[0] < ctx->buflen) ctx->t[1]++;
            blake2b_compress(ctx);
            ctx->buflen = 0;
        }
    }
}

void blake2b_final(blake2b_ctx *ctx, uint8_t *out) {
    ctx->t[0] += ctx->buflen;
    if (ctx->t[0] < ctx->buflen) ctx->t[1]++;
    ctx->f[0] = 0xFFFFFFFFFFFFFFFFULL;
    blake2b_compress(ctx);
    memcpy(out, ctx->h, ctx->outlen);
}

void blake2b(const uint8_t *data, size_t len, uint8_t *out, size_t outlen) {
    blake2b_ctx ctx;
    blake2b_init(&ctx, outlen);
    blake2b_update(&ctx, data, len);
    blake2b_final(&ctx, out);
}

void blake2s_init(blake2s_ctx *ctx, size_t outlen) {
    memset(ctx, 0, sizeof(blake2s_ctx));
    for (int i = 0; i < 8; i++) ctx->h[i] = blake2s_iv[i];
    ctx->h[0] ^= 0x01010000 ^ (uint32_t)outlen;
    ctx->outlen = outlen;
}

void blake2s_update(blake2s_ctx *ctx, const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        ctx->buf[ctx->buflen++] = data[i];
        if (ctx->buflen == 64) {
            ctx->t[0] += ctx->buflen;
            blake2s_compress(ctx);
            ctx->buflen = 0;
        }
    }
}

void blake2s_final(blake2s_ctx *ctx, uint8_t *out) {
    ctx->t[0] += ctx->buflen;
    ctx->f[0] = 0xFFFFFFFFU;
    blake2s_compress(ctx);
    memcpy(out, ctx->h, ctx->outlen);
}

void blake2s(const uint8_t *data, size_t len, uint8_t *out, size_t outlen) {
    blake2s_ctx ctx;
    blake2s_init(&ctx, outlen);
    blake2s_update(&ctx, data, len);
    blake2s_final(&ctx, out);
}
