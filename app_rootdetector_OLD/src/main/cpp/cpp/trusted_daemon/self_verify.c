#include "self_verify.h"
#include "../crypto/core/crypto_core.h"
#include "../bare_syscall/bare_syscall.h"
#include <string.h>

#define NUM_SEGMENTS 8

static uint8_t g_segment_hashes[NUM_SEGMENTS][64];

static int load_segment(int seg_idx, uint8_t *buf, size_t max_size) {
    int fd = bare_open("/proc/self/exe", 0, 0);
    if (fd < 0) return -1;

    size_t seg_size = 4096;
    long offset = (long)seg_idx * (long)seg_size;
    bare_lseek(fd, offset, 0);

    ssize_t n = bare_read_full(fd, buf, seg_size);
    bare_close(fd);
    if (n <= 0) return -1;
    return (int)n;
}

int self_verify_code_crc(void) {
    int fd = bare_open("/proc/self/exe", 0, 0);
    if (fd < 0) return 0;
    char buf[65536];
    ssize_t len = bare_read_full(fd, buf, sizeof(buf));
    bare_close(fd);
    if (len <= 0) return 0;
    return (int)(crc32_simple((const uint8_t *)buf, (size_t)len) & 0x7FFFFFFF);
}

int self_verify_integrity(void) {
    int passed = 1;

    for (int seg = 0; seg < NUM_SEGMENTS; seg++) {
        uint8_t buf[4096];
        uint8_t hash[64];
        int n = load_segment(seg, buf, sizeof(buf));
        if (n <= 0) {
            passed = 0;
            continue;
        }

        sha3_512(buf, (size_t)n, hash);

        if (seg == 0) {
            bare_memcpy(g_segment_hashes[seg], hash, 64);
        } else {
            int diff = 0;
            for (int i = 0; i < 64; i++)
                diff |= g_segment_hashes[seg][i] ^ hash[i];
            if (diff) passed = 0;
        }
    }

    return passed;
}

int self_verify_segment_hash(int seg_idx, uint8_t hash_out[64]) {
    if (seg_idx < 0 || seg_idx >= NUM_SEGMENTS) return -1;

    uint8_t buf[4096];
    int n = load_segment(seg_idx, buf, sizeof(buf));
    if (n <= 0) return -1;

    sha3_512(buf, (size_t)n, hash_out);
    return 0;
}

int self_verify_full_hash(uint8_t hash_out[64]) {
    int fd = bare_open("/proc/self/exe", 0, 0);
    if (fd < 0) return -1;

    sha3_ctx ctx;
    sha3_512_init(&ctx);

    char buf[4096];
    ssize_t n;
    while ((n = bare_read_full(fd, buf, sizeof(buf))) > 0) {
        sha3_512_update(&ctx, (const uint8_t *)buf, (size_t)n);
    }
    bare_close(fd);

    sha3_512_final(&ctx, hash_out);
    return 0;
}
