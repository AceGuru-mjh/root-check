#include "crypto_core.h"
#include <string.h>

static const uint64_t keccakf_rc[24] = {
    0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL,
    0x8000000080008000ULL, 0x000000000000808bULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008aULL,
    0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL,
    0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800aULL, 0x800000008000000aULL, 0x8000000080008081ULL,
    0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL
};

static int keccakf(uint64_t st[25], int rounds) {
    uint64_t bc[5];
    for (int r = 0; r < rounds; r++) {
        bc[0] = st[0] ^ st[5] ^ st[10] ^ st[15] ^ st[20];
        bc[1] = st[1] ^ st[6] ^ st[11] ^ st[16] ^ st[21];
        bc[2] = st[2] ^ st[7] ^ st[12] ^ st[17] ^ st[22];
        bc[3] = st[3] ^ st[8] ^ st[13] ^ st[18] ^ st[23];
        bc[4] = st[4] ^ st[9] ^ st[14] ^ st[19] ^ st[24];
        for (int i = 0; i < 5; i++) {
            uint64_t t = bc[(i + 4) % 5] ^ ((bc[(i + 1) % 5] << 1) | (bc[(i + 1) % 5] >> 63));
            for (int j = 0; j < 25; j += 5) st[j + i] ^= t;
        }
        uint64_t t = st[1];
        for (int i = 0; i < 24; i++) {
            int j = keccakf_rc[i] & 31;
            if (j == 0) continue;
            int offset = ((i + 1) * (i + 2) / 2) % 64;
            uint64_t tmp = st[j];
            st[j] = ((t << offset) | (t >> (64 - offset)));
            t = tmp;
        }
        for (int j = 0; j < 25; j += 5) {
            uint64_t tmp = st[j];
            st[j] ^= st[j + 1] ^ st[j + 2] ^ st[j + 3] ^ st[j + 4];
            st[j + 1] ^= tmp ^ st[j + 2] ^ st[j + 3] ^ st[j + 4];
            st[j + 2] ^= tmp ^ st[j + 1] ^ st[j + 3] ^ st[j + 4];
            st[j + 3] ^= tmp ^ st[j + 1] ^ st[j + 2] ^ st[j + 4];
            st[j + 4] ^= tmp ^ st[j + 1] ^ st[j + 2] ^ st[j + 3];
        }
        st[0] ^= keccakf_rc[r];
    }
    return 1;
}

void sha3_256_init(sha3_ctx *ctx) {
    memset(ctx, 0, sizeof(sha3_ctx));
    ctx->rate = SHA3_256_RATE;
    ctx->capacity = 64;
    ctx->output_len = 32;
    ctx->block_size = ctx->rate;
}

void sha3_256_update(sha3_ctx *ctx, const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        ctx->buf[ctx->buf_len++] = data[i];
        if (ctx->buf_len >= ctx->block_size) {
            for (size_t j = 0; j < ctx->block_size; j++)
                ((uint8_t *)ctx->state)[j] ^= ctx->buf[j];
            keccakf(ctx->state, 24);
            ctx->buf_len = 0;
        }
    }
}

void sha3_256_final(sha3_ctx *ctx, uint8_t *digest) {
    ctx->buf[ctx->buf_len++] = 0x06;
    memset(ctx->buf + ctx->buf_len, 0, ctx->block_size - ctx->buf_len);
    ctx->buf[ctx->block_size - 1] |= 0x80;
    for (size_t j = 0; j < ctx->block_size; j++)
        ((uint8_t *)ctx->state)[j] ^= ctx->buf[j];
    keccakf(ctx->state, 24);
    memcpy(digest, ctx->state, ctx->output_len);
}

void sha3_256(const uint8_t *data, size_t len, uint8_t *digest) {
    sha3_ctx ctx;
    sha3_256_init(&ctx);
    sha3_256_update(&ctx, data, len);
    sha3_256_final(&ctx, digest);
}

void sha3_512_init(sha3_ctx *ctx) {
    memset(ctx, 0, sizeof(sha3_ctx));
    ctx->rate = SHA3_512_RATE;
    ctx->capacity = 64;
    ctx->output_len = 64;
    ctx->block_size = ctx->rate;
}

void sha3_512_update(sha3_ctx *ctx, const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        ctx->buf[ctx->buf_len++] = data[i];
        if (ctx->buf_len >= ctx->block_size) {
            for (size_t j = 0; j < ctx->block_size; j++)
                ((uint8_t *)ctx->state)[j] ^= ctx->buf[j];
            keccakf(ctx->state, 24);
            ctx->buf_len = 0;
        }
    }
}

void sha3_512_final(sha3_ctx *ctx, uint8_t *digest) {
    ctx->buf[ctx->buf_len++] = 0x06;
    memset(ctx->buf + ctx->buf_len, 0, ctx->block_size - ctx->buf_len);
    ctx->buf[ctx->block_size - 1] |= 0x80;
    for (size_t j = 0; j < ctx->block_size; j++)
        ((uint8_t *)ctx->state)[j] ^= ctx->buf[j];
    keccakf(ctx->state, 24);
    memcpy(digest, ctx->state, ctx->output_len);
}

void sha3_512(const uint8_t *data, size_t len, uint8_t *digest) {
    sha3_ctx ctx;
    sha3_512_init(&ctx);
    sha3_512_update(&ctx, data, len);
    sha3_512_final(&ctx, digest);
}