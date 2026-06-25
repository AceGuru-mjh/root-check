#include "../ipc.h"
#include "../../crypto/core/crypto_core.h"
#include "../../bare_syscall/bare_syscall.h"
#include <string.h>
#include <unistd.h>

/* 安全随机数填充：使用 getrandom 系统调用 */
static void secure_random_fill(uint8_t *buf, size_t len) {
    long ret = bare_syscall(278, buf, len, 0);  /* __NR_getrandom */
    if (ret < 0) {
        /* 回退：混合 RDTSC + PID */
        for (size_t i = 0; i < len; i += 8) {
            uint64_t val = bare_rdtsc() ^ ((uint64_t)bare_getpid() << 32);
            size_t copy = (len - i) < 8 ? (len - i) : 8;
            bare_memcpy(buf + i, &val, copy);
        }
    }
}

int ipc_encrypt_payload(const uint8_t *plain, uint32_t plain_len,
                        const uint8_t *key, uint8_t *cipher,
                        uint8_t *tag, uint8_t *iv) {
    secure_random_fill(iv, 12);
    return aes256_gcm_encrypt(plain, plain_len, key, iv, cipher, tag);
}

int ipc_decrypt_payload(const uint8_t *cipher, uint32_t cipher_len,
                        const uint8_t *key, const uint8_t *iv,
                        const uint8_t *tag, uint8_t *plain) {
    return aes256_gcm_decrypt(cipher, cipher_len, key, iv, tag, plain);
}

void ipc_compute_mac(const uint8_t *data, uint32_t data_len,
                     const uint8_t *key, uint8_t mac[16]) {
    uint8_t input[64 + IPC_MAX_MSG_SIZE];
    uint32_t input_len = 32 > data_len ? 32 : data_len;
    memcpy(input, key, 32);
    memcpy(input + 32, data, data_len < IPC_MAX_MSG_SIZE ? data_len : IPC_MAX_MSG_SIZE);
    sha3_256(input, 32 + data_len, mac);
}

int ipc_verify_mac(const uint8_t *data, uint32_t data_len,
                   const uint8_t *key, const uint8_t mac[16]) {
    uint8_t computed[32];
    ipc_compute_mac(data, data_len, key, computed);
    return memcmp(computed, mac, 16) == 0 ? 0 : -1;
}