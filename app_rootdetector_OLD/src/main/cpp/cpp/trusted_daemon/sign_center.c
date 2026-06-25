#include "sign_center.h"
#include "../crypto/core/crypto_core.h"
#include "../bare_syscall/bare_syscall.h"
#include <string.h>

/* 设备唯一密钥（域C内置，用于全域结果签名）*/
static uint8_t g_device_key[32];
static uint8_t g_device_id[32];
static int g_initialized = 0;

void sign_center_init(const uint8_t *device_key, const uint8_t *device_id) {
    if (device_key) bare_memcpy(g_device_key, device_key, 32);
    if (device_id) bare_memcpy(g_device_id, device_id, 32);
    g_initialized = 1;
}

int sign_center_sign(const uint8_t *data, size_t len,
                     uint8_t signature[64]) {
    if (!g_initialized || !data || !signature) return -1;

    sha3_ctx ctx;
    uint8_t inner[64];
    uint8_t key_ipad[32], key_opad[32];

    for (int i = 0; i < 32; i++) {
        key_ipad[i] = g_device_key[i] ^ 0x36;
        key_opad[i] = g_device_key[i] ^ 0x5c;
    }

    sha3_512_init(&ctx);
    sha3_512_update(&ctx, key_ipad, 32);
    sha3_512_final(&ctx, inner);

    sha3_512_init(&ctx);
    sha3_512_update(&ctx, inner, 64);
    sha3_512_update(&ctx, data, len);
    sha3_512_final(&ctx, inner);

    sha3_512_init(&ctx);
    sha3_512_update(&ctx, key_opad, 32);
    sha3_512_final(&ctx, key_opad);

    sha3_512_init(&ctx);
    sha3_512_update(&ctx, key_opad, 64);
    sha3_512_update(&ctx, inner, 64);
    sha3_512_final(&ctx, signature);

    return 0;
}

int sign_center_verify(const uint8_t *data, size_t len,
                       const uint8_t signature[64]) {
    if (!g_initialized || !data || !signature) return 0;

    uint8_t expected[64];
    if (sign_center_sign(data, len, expected) != 0) return 0;

    int diff = 0;
    for (int i = 0; i < 64; i++)
        diff |= expected[i] ^ signature[i];
    return diff == 0 ? 1 : 0;
}
