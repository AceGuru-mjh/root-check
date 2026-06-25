#include "crypto_core.h"
#include <string.h>

static const uint32_t rcon[] = {
    0x01000000, 0x02000000, 0x04000000, 0x08000000, 0x10000000,
    0x20000000, 0x40000000, 0x80000000, 0x1B000000, 0x36000000
};

static const uint8_t sbox[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static uint32_t sub_word(uint32_t w) {
    return (sbox[w >> 24] << 24) | (sbox[(w >> 16) & 0xff] << 16) |
           (sbox[(w >> 8) & 0xff] << 8) | sbox[w & 0xff];
}

static uint32_t rot_word(uint32_t w) {
    return (w << 8) | (w >> 24);
}

void aes256_init(aes256_ctx *ctx, const uint8_t *key) {
    uint32_t *rk = ctx->rk;
    for (int i = 0; i < 8; i++, key += 4)
        rk[i] = (key[0] << 24) | (key[1] << 16) | (key[2] << 8) | key[3];
    for (int i = 8; i < 60; i++) {
        uint32_t temp = rk[i - 1];
        if (i % 8 == 0) temp = sub_word(rot_word(temp)) ^ rcon[i / 8 - 1];
        else if (i % 8 == 4) temp = sub_word(temp);
        rk[i] = rk[i - 8] ^ temp;
    }
    ctx->rounds = 14;
}

static void add_round_key(uint8_t *state, const uint32_t *rk) {
    for (int i = 0; i < 4; i++) {
        uint32_t k = rk[i];
        state[4*i] ^= (k >> 24) & 0xff;
        state[4*i+1] ^= (k >> 16) & 0xff;
        state[4*i+2] ^= (k >> 8) & 0xff;
        state[4*i+3] ^= k & 0xff;
    }
}

static void sub_bytes(uint8_t *state) {
    for (int i = 0; i < 16; i++) state[i] = sbox[state[i]];
}

static void shift_rows(uint8_t *state) {
    uint8_t tmp;
    tmp = state[1]; state[1] = state[5]; state[5] = state[9]; state[9] = state[13]; state[13] = tmp;
    tmp = state[2]; state[2] = state[10]; state[10] = tmp;
    tmp = state[6]; state[6] = state[14]; state[14] = tmp;
    tmp = state[3]; state[3] = state[15]; state[15] = state[11]; state[11] = state[7]; state[7] = tmp;
}

static uint8_t gmul(uint8_t a, uint8_t b) {
    uint8_t p = 0;
    for (int i = 0; i < 8; i++) {
        if (b & 1) p ^= a;
        uint8_t hi = a & 0x80;
        a <<= 1;
        if (hi) a ^= 0x1b;
        b >>= 1;
    }
    return p;
}

static void mix_columns(uint8_t *state) {
    for (int i = 0; i < 4; i++) {
        uint8_t *col = state + 4 * i;
        uint8_t a[4];
        memcpy(a, col, 4);
        col[0] = gmul(a[0], 2) ^ gmul(a[1], 3) ^ a[2] ^ a[3];
        col[1] = a[0] ^ gmul(a[1], 2) ^ gmul(a[2], 3) ^ a[3];
        col[2] = a[0] ^ a[1] ^ gmul(a[2], 2) ^ gmul(a[3], 3);
        col[3] = gmul(a[0], 3) ^ a[1] ^ a[2] ^ gmul(a[3], 2);
    }
}

void aes256_encrypt_block(aes256_ctx *ctx, const uint8_t *in, uint8_t *out) {
    uint8_t state[16];
    memcpy(state, in, 16);
    add_round_key(state, ctx->rk);
    for (int r = 1; r < 14; r++) {
        sub_bytes(state);
        shift_rows(state);
        mix_columns(state);
        add_round_key(state, ctx->rk + r * 4);
    }
    sub_bytes(state);
    shift_rows(state);
    add_round_key(state, ctx->rk + 14 * 4);
    memcpy(out, state, 16);
}

static void gcm_inc32(uint8_t *block) {
    for (int i = 15; i >= 12; i--)
        if (++block[i]) break;
}

static void gcm_ghash(uint8_t *out, const uint8_t *h, const uint8_t *a, size_t a_len,
                      const uint8_t *c, size_t c_len) {
    uint8_t y[16] = {0};
    size_t i;
    for (i = 0; i + 16 <= a_len; i += 16) {
        for (int j = 0; j < 16; j++) y[j] ^= a[i + j];
    }
    if (i < a_len) {
        for (size_t j = 0; j < a_len - i; j++) y[j] ^= a[i + j];
    }
    for (i = 0; i + 16 <= c_len; i += 16) {
        for (int j = 0; j < 16; j++) y[j] ^= c[i + j];
    }
    if (i < c_len) {
        for (size_t j = 0; j < c_len - i; j++) y[j] ^= c[i + j];
    }
    uint64_t bits_a = (uint64_t)a_len * 8;
    uint64_t bits_c = (uint64_t)c_len * 8;
    for (int j = 0; j < 8; j++) y[j] ^= (bits_a >> (56 - j * 8)) & 0xff;
    for (int j = 0; j < 8; j++) y[8 + j] ^= (bits_c >> (56 - j * 8)) & 0xff;
    memcpy(out, y, 16);
}

int aes256_gcm_encrypt(const uint8_t *plain, size_t plain_len,
                       const uint8_t *key, const uint8_t *iv,
                       uint8_t *cipher, uint8_t *tag) {
    aes256_ctx ctx;
    aes256_init(&ctx, key);
    uint8_t j0[16], h[16];
    uint8_t zero[16] = {0};
    aes256_encrypt_block(&ctx, zero, h);
    if (iv && iv[0] == 0) {
        memcpy(j0, iv + 1, 15);
        j0[15] = 1;
    } else {
        memcpy(j0, iv, 12);
        j0[12] = j0[13] = j0[14] = 0;
        j0[15] = 1;
    }
    uint8_t cb[16];
    memcpy(cb, j0, 16);
    for (size_t i = 0; i + 16 <= plain_len; i += 16) {
        gcm_inc32(cb);
        aes256_encrypt_block(&ctx, cb, cipher + i);
        for (int j = 0; j < 16; j++) cipher[i + j] ^= plain[i + j];
    }
    size_t rem = plain_len % 16;
    if (rem) {
        size_t off = plain_len - rem;
        gcm_inc32(cb);
        uint8_t ek[16];
        aes256_encrypt_block(&ctx, cb, ek);
        for (size_t j = 0; j < rem; j++) cipher[off + j] = plain[off + j] ^ ek[j];
    }
    uint8_t s[16];
    aes256_encrypt_block(&ctx, j0, s);
    gcm_ghash(tag, h, NULL, 0, cipher, plain_len);
    for (int j = 0; j < 16; j++) tag[j] ^= s[j];
    return 0;
}

int aes256_gcm_decrypt(const uint8_t *cipher, size_t cipher_len,
                       const uint8_t *key, const uint8_t *iv,
                       const uint8_t *tag, uint8_t *plain) {
    aes256_ctx ctx;
    aes256_init(&ctx, key);
    uint8_t j0[16], h[16];
    uint8_t zero[16] = {0};
    aes256_encrypt_block(&ctx, zero, h);
    if (iv && iv[0] == 0) {
        memcpy(j0, iv + 1, 15);
        j0[15] = 1;
    } else {
        memcpy(j0, iv, 12);
        j0[12] = j0[13] = j0[14] = 0;
        j0[15] = 1;
    }
    uint8_t expected_tag[16], s[16];
    aes256_encrypt_block(&ctx, j0, s);
    gcm_ghash(expected_tag, h, NULL, 0, cipher, cipher_len);
    for (int j = 0; j < 16; j++) expected_tag[j] ^= s[j];
    if (memcmp(expected_tag, tag, 16) != 0) return -1;
    uint8_t cb[16];
    memcpy(cb, j0, 16);
    for (size_t i = 0; i + 16 <= cipher_len; i += 16) {
        gcm_inc32(cb);
        aes256_encrypt_block(&ctx, cb, plain + i);
        for (int j = 0; j < 16; j++) plain[i + j] ^= cipher[i + j];
    }
    size_t rem = cipher_len % 16;
    if (rem) {
        size_t off = cipher_len - rem;
        gcm_inc32(cb);
        uint8_t ek[16];
        aes256_encrypt_block(&ctx, cb, ek);
        for (size_t j = 0; j < rem; j++) plain[off + j] = cipher[off + j] ^ ek[j];
    }
    return 0;
}