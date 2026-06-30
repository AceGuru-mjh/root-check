#include "utils.h"
#include "syscall.h"
#include <cstring>
#include <fcntl.h>
#include <sys/types.h>

namespace apex {
namespace utils {

bool file_exists(const char* path) {
    int64_t ret = 0;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; svc #0; mov %0, x0"
                 : "=r"(ret)
                 : "i"(__NR_access), "r"(path), "i"(F_OK)
                 : "x0", "x1", "x8");
    return ret == 0;
}

ssize_t read_file(const char* path, char* buf, size_t size) {
    int64_t fd = 0;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd)
                 : "i"(__NR_openat), "i"(AT_FDCWD), "r"(path), "i"(O_RDONLY), "i"(0)
                 : "x0", "x1", "x2", "x8");
    if (fd < 0) return -1;

    char tmp[4096];
    int64_t n = 0;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n)
                 : "i"(__NR_read), "r"(fd), "r"(tmp), "r"((int64_t)(size < 4096 ? size : 4096))
                 : "x0", "x1", "x2", "x8");

    int64_t close_ret;
    asm volatile("mov x8, %1; mov x0, %2; svc #0; mov %0, x0"
                 : "=r"(close_ret) : "i"(__NR_close), "r"(fd) : "x0", "x8");
    if (n > 0) {
        size_t copy = n < (int64_t)size ? n : (int64_t)size;
        std::memcpy(buf, tmp, copy);
    }
    return n;
}

bool write_file(const char* path, const char* content, size_t len) {
    int flags = O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC;
    int mode = 0644;
    int64_t fd = 0;
    asm volatile("mov x8, %[nr]; mov x0, %[dir]; mov x1, %[path]; mov x2, %[flags]; mov x3, %[mode]; svc #0; mov %[fd], x0"
                 : [fd] "=r"(fd)
                 : [nr] "i"(__NR_openat), [dir] "i"(AT_FDCWD), [path] "r"(path), [flags] "r"((int64_t)flags), [mode] "r"((int64_t)mode)
                 : "x0", "x1", "x2", "x3", "x8");
    if (fd < 0) return false;

    int64_t n;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_write), "r"(fd), "r"(content), "r"((int64_t)len)
                 : "x0", "x1", "x2", "x8");

    int64_t close_ret;
    asm volatile("mov x8, %1; mov x0, %2; svc #0; mov %0, x0"
                 : "=r"(close_ret) : "i"(__NR_close), "r"(fd) : "x0", "x8");
    return n == (int64_t)len;
}

bool delete_path(const char* path) {
    int64_t ret = 0;
    asm volatile("mov x8, 10; mov x0, %1; svc #0; mov %0, x0" // unlinkat
                 : "=r"(ret) : "r"(path) : "x0", "x8");
    return ret == 0;
}

bool exec_su_command(const char* cmd) {
    int64_t pid = 0;
    asm volatile("mov x8, %1; svc #0; mov %0, x0"
                 : "=r"(pid) : "i"(__NR_fork) : "x0", "x8");
    if (pid == 0) {
        const char* argv[] = {"su", "-c", cmd, nullptr};
        const char* envp[] = {"PATH=/sbin:/system/bin:/system/xbin", nullptr};
        asm volatile("mov x8, 221; mov x0, %0; mov x1, %1; mov x2, %2; svc #0" // execve
                     : : "r"("/system/bin/sh"), "r"(argv), "r"(envp) : "x0", "x1", "x2", "x8");
        asm volatile("mov x8, 93; mov x0, 1; svc #0"); // _exit
    }
    return pid >= 0;
}

bool exec_su_command_quiet(const char* cmd) {
    int64_t pid = 0;
    asm volatile("mov x8, %1; svc #0; mov %0, x0"
                 : "=r"(pid) : "i"(__NR_fork) : "x0", "x8");
    if (pid == 0) {
        int64_t devnull;
        asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                     : "=r"(devnull) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/dev/null"), "i"(O_RDWR), "i"(0));
        int64_t null_fd = devnull;
        asm volatile("mov x8, 24; mov x0, %1; mov x1, %2; svc #0" : : "i"(__NR_dup3), "r"(null_fd), "i"(0) : "x0", "x8");
        asm volatile("mov x8, 24; mov x0, %1; mov x1, %2; svc #0" : : "i"(__NR_dup3), "r"(null_fd), "i"(1) : "x0", "x8");
        asm volatile("mov x8, 24; mov x0, %1; mov x1, %2; svc #0" : : "i"(__NR_dup3), "r"(null_fd), "i"(2) : "x0", "x8");
        if (devnull >= 0) {
            int64_t cr;
            asm volatile("mov x8, %1; mov x0, %2; svc #0; mov %0, x0" : "=r"(cr) : "i"(__NR_close), "r"(null_fd) : "x0", "x8");
        }
        const char* argv[] = {"sh", "-c", cmd, nullptr};
        asm volatile("mov x8, 221; mov x0, %0; mov x1, %1; mov x2, %2; svc #0"
                     : : "r"("/system/bin/sh"), "r"(argv), "r"(nullptr) : "x0", "x1", "x2", "x8");
        asm volatile("mov x8, 93; mov x0, 1; svc #0");
    }
    return pid >= 0;
}

static void hex_encode(const uint8_t* in, size_t len, char* out) {
    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < len; i++) {
        out[i*2] = hex[(in[i] >> 4) & 0xf];
        out[i*2+1] = hex[in[i] & 0xf];
    }
    out[len*2] = '\0';
}

std::string sha256_hash(const uint8_t* data, size_t len) {
    // SHA-256 using software implementation
    uint32_t h[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    static const uint32_t K[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    };

    uint32_t w[64];
    size_t offset = 0;
    while (offset + 64 <= len) {
        for (int i = 0; i < 16; i++)
            w[i] = (data[offset + i*4] << 24) | (data[offset + i*4+1] << 16) |
                   (data[offset + i*4+2] << 8) | data[offset + i*4+3];
        for (int i = 16; i < 64; i++) {
            uint32_t s0 = ((w[i-15] >> 7) | (w[i-15] << 25)) ^ ((w[i-15] >> 18) | (w[i-15] << 14)) ^ (w[i-15] >> 3);
            uint32_t s1 = ((w[i-2] >> 17) | (w[i-2] << 15)) ^ ((w[i-2] >> 19) | (w[i-2] << 13)) ^ (w[i-2] >> 10);
            w[i] = w[i-16] + w[i-7] + s0 + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; i++) {
            uint32_t S1 = ((e >> 6) | (e << 26)) ^ ((e >> 11) | (e << 21)) ^ ((e >> 25) | (e << 7));
            uint32_t ch = (e & f) ^ ((~e) & g);
            uint32_t temp1 = hh + S1 + ch + K[i] + w[i];
            uint32_t S0 = ((a >> 2) | (a << 30)) ^ ((a >> 13) | (a << 19)) ^ ((a >> 22) | (a << 10));
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = S0 + maj;
            hh = g; g = f; f = e; e = d + temp1;
            d = c; c = b; b = a; a = temp1 + temp2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
        offset += 64;
    }

    // Padding block
    uint8_t pad[128];
    std::memcpy(pad, data + offset, len - offset);
    size_t pad_len = len - offset;
    pad[pad_len] = 0x80;
    pad_len++;
    if (pad_len > 56) {
        std::memset(pad + pad_len, 0, 128 - pad_len);
        for (int i = 0; i < 16; i++)
            w[i] = (pad[i*4] << 24) | (pad[i*4+1] << 16) | (pad[i*4+2] << 8) | pad[i*4+3];
        for (int i = 16; i < 64; i++) {
            uint32_t s0 = ((w[i-15] >> 7) | (w[i-15] << 25)) ^ ((w[i-15] >> 18) | (w[i-15] << 14)) ^ (w[i-15] >> 3);
            uint32_t s1 = ((w[i-2] >> 17) | (w[i-2] << 15)) ^ ((w[i-2] >> 19) | (w[i-2] << 13)) ^ (w[i-2] >> 10);
            w[i] = w[i-16] + w[i-7] + s0 + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; i++) {
            uint32_t S1 = ((e >> 6) | (e << 26)) ^ ((e >> 11) | (e << 21)) ^ ((e >> 25) | (e << 7));
            uint32_t ch = (e & f) ^ ((~e) & g);
            uint32_t temp1 = hh + S1 + ch + K[i] + w[i];
            uint32_t S0 = ((a >> 2) | (a << 30)) ^ ((a >> 13) | (a << 19)) ^ ((a >> 22) | (a << 10));
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = S0 + maj;
            hh = g; g = f; f = e; e = d + temp1;
            d = c; c = b; b = a; a = temp1 + temp2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
        pad_len = 0;
    }

    std::memset(pad + pad_len, 0, 56 - pad_len);
    uint64_t bit_len = (uint64_t)len * 8;
    for (int i = 0; i < 8; i++)
        pad[56 + pad_len + i] = (bit_len >> (56 - i * 8)) & 0xFF;

    for (int i = 0; i < 16; i++)
        w[i] = (pad[i*4] << 24) | (pad[i*4+1] << 16) | (pad[i*4+2] << 8) | pad[i*4+3];
    for (int i = 16; i < 64; i++) {
        uint32_t s0 = ((w[i-15] >> 7) | (w[i-15] << 25)) ^ ((w[i-15] >> 18) | (w[i-15] << 14)) ^ (w[i-15] >> 3);
        uint32_t s1 = ((w[i-2] >> 17) | (w[i-2] << 15)) ^ ((w[i-2] >> 19) | (w[i-2] << 13)) ^ (w[i-2] >> 10);
        w[i] = w[i-16] + w[i-7] + s0 + s1;
    }
    uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
    uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
    for (int i = 0; i < 64; i++) {
        uint32_t S1 = ((e >> 6) | (e << 26)) ^ ((e >> 11) | (e << 21)) ^ ((e >> 25) | (e << 7));
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t temp1 = hh + S1 + ch + K[i] + w[i];
        uint32_t S0 = ((a >> 2) | (a << 30)) ^ ((a >> 13) | (a << 19)) ^ ((a >> 22) | (a << 10));
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = S0 + maj;
        hh = g; g = f; f = e; e = d + temp1;
        d = c; c = b; b = a; a = temp1 + temp2;
    }
    h[0] += a; h[1] += b; h[2] += c; h[3] += d;
    h[4] += e; h[5] += f; h[6] += g; h[7] += hh;

    static const char hex[] = "0123456789abcdef";
    char result[65];
    for (int i = 0; i < 8; i++) {
        int idx = i * 8;
        result[idx]   = hex[(h[i] >> 28) & 0xf];
        result[idx+1] = hex[(h[i] >> 24) & 0xf];
        result[idx+2] = hex[(h[i] >> 20) & 0xf];
        result[idx+3] = hex[(h[i] >> 16) & 0xf];
        result[idx+4] = hex[(h[i] >> 12) & 0xf];
        result[idx+5] = hex[(h[i] >> 8) & 0xf];
        result[idx+6] = hex[(h[i] >> 4) & 0xf];
        result[idx+7] = hex[h[i] & 0xf];
    }
    result[64] = '\0';
    return std::string(result);
}

} // namespace utils
} // namespace apex
