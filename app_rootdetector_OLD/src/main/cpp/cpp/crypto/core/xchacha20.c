#include "xchacha20.h"
#include <string.h>

static uint32_t rotl32(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

static void quarter_round(uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {
    *a += *b; *d ^= *a; *d = rotl32(*d, 16);
    *c += *d; *b ^= *c; *b = rotl32(*b, 12);
    *a += *b; *d ^= *a; *d = rotl32(*d, 8);
    *c += *d; *b ^= *c; *b = rotl32(*b, 7);
}

static void double_round(uint32_t s[16]) {
    quarter_round(&s[0], &s[4], &s[8],  &s[12]);
    quarter_round(&s[1], &s[5], &s[9],  &s[13]);
    quarter_round(&s[2], &s[6], &s[10], &s[14]);
    quarter_round(&s[3], &s[7], &s[11], &s[15]);
    quarter_round(&s[0], &s[5], &s[10], &s[15]);
    quarter_round(&s[1], &s[6], &s[11], &s[12]);
    quarter_round(&s[2], &s[7], &s[8],  &s[13]);
    quarter_round(&s[3], &s[4], &s[9],  &s[14]);
}

static void block_function(uint32_t s[16], uint32_t out[16]) {
    uint32_t working[16];
    memcpy(working, s, 64);
    for (int i = 0; i < 10; i++) double_round(working);
    for (int i = 0; i < 16; i++) out[i] = working[i] + s[i];
}

static void hchacha20(const uint8_t key[32], const uint8_t in[16],
                      uint8_t out[32]) {
    uint32_t s[16];
    const char *sigma = "expand 32-byte k";
    for (int i = 0; i < 4; i++) s[i] = ((uint32_t *)sigma)[i];
    for (int i = 0; i < 8; i++) s[4+i] = ((uint32_t *)key)[i];
    for (int i = 0; i < 4; i++) s[12+i] = ((uint32_t *)in)[i];

    uint32_t out32[16];
    block_function(s, out32);
    memcpy(out, out32, 32);
}

void xchacha20_init(xchacha20_ctx *ctx,
                    const uint8_t key[32],
                    const uint8_t nonce[24]) {
    memcpy(ctx->key, key, 32);
    memcpy(ctx->nonce, nonce, 24);
    ctx->counter = 0;

    uint8_t subkey[32];
    hchacha20(key, nonce, subkey);

    const char *sigma = "expand 32-byte k";
    for (int i = 0; i < 4; i++) ctx->state[i] = ((uint32_t *)sigma)[i];
    for (int i = 0; i < 8; i++) ctx->state[4+i] = ((uint32_t *)subkey)[i];
    ctx->state[12] = ctx->counter;
    ctx->state[13] = ((uint32_t *)(nonce + 16))[0];
    ctx->state[14] = ((uint32_t *)(nonce + 16))[1];
    ctx->state[15] = 0;
}

static void encrypt_block(xchacha20_ctx *ctx, const uint8_t *in,
                          uint8_t *out) {
    uint32_t keystream[16];
    block_function(ctx->state, keystream);
    for (int i = 0; i < 16; i++) {
        ((uint32_t *)out)[i] = ((uint32_t *)in)[i] ^ keystream[i];
    }
    ctx->state[12]++;
    if (ctx->state[12] == 0) ctx->state[13]++;
}

void xchacha20_encrypt(xchacha20_ctx *ctx,
                       const uint8_t *plain, uint8_t *cipher, size_t len) {
    size_t offset = 0;
    uint8_t block[XCHACHA20_BLOCK_SIZE];
    while (offset + XCHACHA20_BLOCK_SIZE <= len) {
        encrypt_block(ctx, plain + offset, cipher + offset);
        offset += XCHACHA20_BLOCK_SIZE;
    }
    if (offset < len) {
        uint8_t tmp[XCHACHA20_BLOCK_SIZE];
        memset(tmp, 0, XCHACHA20_BLOCK_SIZE);
        memcpy(tmp, plain + offset, len - offset);
        encrypt_block(ctx, tmp, block);
        memcpy(cipher + offset, block, len - offset);
    }
}

void xchacha20_decrypt(xchacha20_ctx *ctx,
                       const uint8_t *cipher, uint8_t *plain, size_t len) {
    xchacha20_encrypt(ctx, cipher, plain, len);
}
