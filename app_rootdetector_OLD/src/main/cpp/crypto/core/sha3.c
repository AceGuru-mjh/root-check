#include "sha3.h"
#include <string.h>

/* Keccak-f[1600] round constants */
static const uint64_t keccak_rc[24] = {
    0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL,
    0x8000000080008000ULL, 0x000000000000808bULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008aULL,
    0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL,
    0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800aULL, 0x800000008000000aULL, 0x8000000080008081ULL,
    0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL
};

/* Rotation offsets */
static const int keccak_rotc[24] = {
    1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14,
    27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44
};

/* Pi lane indices */
static const int keccak_piln[24] = {
    10, 7,  11, 17, 18, 3,  5,  16, 8,  21, 24, 4,
    15, 23, 19, 13, 12, 2,  20, 14, 22, 9,  6,  1
};

/* Keccak-f[1600] permutation - direct implementation, no function pointers */
static void keccak_f1600(uint64_t state[25]) {
    uint64_t t, bc[5];
    int i, j, round;

    for (round = 0; round < 24; round++) {
        /* Theta */
        for (i = 0; i < 5; i++) {
            bc[i] = state[i] ^ state[i + 5] ^ state[i + 10] ^ state[i + 15] ^ state[i + 20];
        }
        for (i = 0; i < 5; i++) {
            t = bc[(i + 4) % 5] ^ ((bc[(i + 1) % 5] << 1) | (bc[(i + 1) % 5] >> 63));
            for (j = 0; j < 25; j += 5) {
                state[j + i] ^= t;
            }
        }

        /* Rho and Pi */
        t = state[1];
        for (i = 0; i < 24; i++) {
            j = keccak_piln[i];
            bc[0] = state[j];
            state[j] = (t << keccak_rotc[i]) | (t >> (64 - keccak_rotc[i]));
            t = bc[0];
        }

        /* Chi */
        for (j = 0; j < 25; j += 5) {
            for (i = 0; i < 5; i++) {
                bc[i] = state[j + i];
            }
            for (i = 0; i < 5; i++) {
                state[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
            }
        }

        /* Iota */
        state[0] ^= keccak_rc[round];
    }
}

CRYPTO_EXPORT int sha3_512_init(sha3_512_ctx *ctx) {
    if (!ctx) return -1;
    memset(ctx, 0, sizeof(sha3_512_ctx));
    return 0;
}

CRYPTO_EXPORT int sha3_512_update(sha3_512_ctx *ctx, const uint8_t *data, size_t len) {
    size_t i;
    if (!ctx || (!data && len > 0)) return -1;

    for (i = 0; i < len; i++) {
        ctx->buf[ctx->buf_len++] = data[i];
        if (ctx->buf_len == SHA3_512_RATE) {
            /* XOR buffer into state */
            for (size_t j = 0; j < SHA3_512_RATE / 8; j++) {
                uint64_t lane;
                memcpy(&lane, ctx->buf + j * 8, 8);
                ctx->state[j] ^= lane;
            }
            keccak_f1600(ctx->state);
            ctx->buf_len = 0;
        }
    }
    return 0;
}

CRYPTO_EXPORT int sha3_512_final(sha3_512_ctx *ctx, uint8_t *hash) {
    size_t i;
    if (!ctx || !hash) return -1;

    /* SHA3 padding: 0x06 ... 0x80 */
    ctx->buf[ctx->buf_len++] = 0x06;
    while (ctx->buf_len < SHA3_512_RATE - 1) {
        ctx->buf[ctx->buf_len++] = 0x00;
    }
    ctx->buf[SHA3_512_RATE - 1] = 0x80;

    /* XOR final padded block into state */
    for (i = 0; i < SHA3_512_RATE / 8; i++) {
        uint64_t lane;
        memcpy(&lane, ctx->buf + i * 8, 8);
        ctx->state[i] ^= lane;
    }
    keccak_f1600(ctx->state);

    /* Squeeze output (64 bytes = 8 lanes) */
    for (i = 0; i < 8; i++) {
        memcpy(hash + i * 8, &ctx->state[i], 8);
    }

    /* Clear sensitive data */
    memset(ctx, 0, sizeof(sha3_512_ctx));
    return 0;
}

CRYPTO_EXPORT int sha3_512_hash(const uint8_t *data, size_t len, uint8_t *hash) {
    sha3_512_ctx ctx;
    int ret;

    if (!hash) return -1;
    if (!data && len > 0) return -1;

    ret = sha3_512_init(&ctx);
    if (ret != 0) return ret;

    ret = sha3_512_update(&ctx, data, len);
    if (ret != 0) return ret;

    return sha3_512_final(&ctx, hash);
}
