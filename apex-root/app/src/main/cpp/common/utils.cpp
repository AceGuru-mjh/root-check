#include "utils.h"
#include "syscall.h"
#include "../bare_syscall/syscall_bridge.h"
#include <cstring>
#include <cctype>
#include <fcntl.h>
#include <sys/types.h>
#include <mutex>    // std::call_once / std::once_flag (P3-10 maps cache)
#include <string>   // std::string (P3-10 maps cache)

namespace apex {
namespace utils {

// ─────────────────────────────────────────────────────────────
// v1.1.3 P2-S1: 命令注入防御
// ─────────────────────────────────────────────────────────────
// exec_su_command_quiet 实际通过 `sh -c "<cmd>"` 执行, 如果 cmd 中嵌入了
// 用户/外部输入(如 serialno、sandbox_name、文件路径), 攻击者可注入 `;`、
// `$(...)`、反引号 等元字符执行任意命令。这里加两层防御:
//
//   1. is_cmd_safe(cmd): 拒绝包含命令替换(`$(...)`/`` `...` ``)的整条命令。
//      注意: 不拒绝 `;` `&&` `||` `2>/dev/null`, 因为正常复合命令也会用到
//      (例如 `rmmod kernelsu 2>/dev/null`)。
//
//   2. is_safe_value(s): 严格白名单, 只允许 [a-zA-Z0-9._-]。
//      调用方在把变量拼进 cmd 前, 必须先用 is_safe_value 校验。
//
// 这两层防御共同保证: 即使调用方漏检, exec_su_command_quiet 仍会拒绝明显
// 含命令替换的命令; 而对单值参数, 调用方应主动调用 is_safe_value。
// ─────────────────────────────────────────────────────────────

bool is_safe_value(const char* s) {
    if (!s || !*s) return false;
    for (const char* p = s; *p; p++) {
        // 只允许 [a-zA-Z0-9._-], 拒绝 ' " ; $ ` ( ) < > | & 空格 等
        if (!(std::isalnum(static_cast<unsigned char>(*p)) ||
              *p == '.' || *p == '_' || *p == '-')) {
            return false;
        }
    }
    return true;
}

bool is_safe_path(const char* s) {
    if (!s || !*s) return false;
    for (const char* p = s; *p; p++) {
        // 允许 [a-zA-Z0-9._/-], 拒绝所有 shell 元字符
        if (!(std::isalnum(static_cast<unsigned char>(*p)) ||
              *p == '.' || *p == '_' || *p == '-' || *p == '/')) {
            return false;
        }
    }
    return true;
}

static bool is_cmd_safe(const char* cmd) {
    if (!cmd || !*cmd) return false;
    // 拒绝命令替换模式 (注入的主要途径)
    // 不拒绝 ; && || 等复合命令操作符 — 正常 shell 命令也会用到
    static const char* dangerous[] = {
        "$(",
        "`",
        nullptr
    };
    for (int i = 0; dangerous[i]; i++) {
        if (std::strstr(cmd, dangerous[i])) return false;
    }
    return true;
}

bool file_exists(const char* path) {
    int64_t ret = 0;
    ret = apex_check_access(path);
    return ret == 0;
}

ssize_t read_file(const char* path, char* buf, size_t size) {
    int64_t fd = 0;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd)
                 : "i"(__NR_openat), "i"(AT_FDCWD), "r"(path), "i"(O_RDONLY), "i"(0)
                 : "x0", "x1", "x2", "x8", "memory");
    #else
        /* arm32/x64 fallback */ (void)0;
    #endif
    if (fd < 0) return -1;

    char tmp[4096];
    int64_t n = 0;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n)
                 : "i"(__NR_read), "r"(fd), "r"(tmp), "r"((int64_t)(size < 4096 ? size : 4096))
                 : "x0", "x1", "x2", "x8", "memory");
    #else
        /* arm32/x64 fallback */ (void)0;
    #endif

    int64_t close_ret;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; svc #0; mov %0, x0"
                 : "=r"(close_ret) : "i"(__NR_close), "r"(fd) : "x0", "x8", "memory");
    #else
        close_ret = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
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
    #if defined(__aarch64__)
    asm volatile("mov x8, %[nr]; mov x0, %[dir]; mov x1, %[path]; mov x2, %[flags]; mov x3, %[mode]; svc #0; mov %[fd], x0"
                 : [fd] "=r"(fd)
                 : [nr] "i"(__NR_openat), [dir] "i"(AT_FDCWD), [path] "r"(path), [flags] "r"((int64_t)flags), [mode] "r"((int64_t)mode)
                 : "x0", "x1", "x2", "x3", "x8", "memory");
    #else
        /* arm32/x64 fallback */ (void)0;
    #endif
    if (fd < 0) return false;

    int64_t n;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(n) : "i"(__NR_write), "r"(fd), "r"(content), "r"((int64_t)len)
                 : "x0", "x1", "x2", "x8", "memory");
    #else
        n = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif

    int64_t close_ret;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; svc #0; mov %0, x0"
                 : "=r"(close_ret) : "i"(__NR_close), "r"(fd) : "x0", "x8", "memory");
    #else
        close_ret = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    return n == (int64_t)len;
}

bool delete_path(const char* path) {
    int64_t ret = 0;
    #if defined(__aarch64__)
    asm volatile("mov x8, 10; mov x0, %1; svc #0; mov %0, x0" // unlinkat
                 : "=r"(ret) : "r"(path) : "x0", "x8", "memory");
    #else
        /* arm32/x64 fallback */ (void)0;
    #endif
    return ret == 0;
}

bool exec_su_command(const char* cmd) {
    // v1.1.3 P2-S1: 拒绝含命令替换模式的命令 (防御性, 调用方仍应避免拼接)
    if (!is_cmd_safe(cmd)) return false;
    int64_t pid = 0;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; svc #0; mov %0, x0"
                 : "=r"(pid) : "i"(__NR_fork) : "x0", "x8", "memory");
    #else
        pid = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (pid == 0) {
        const char* argv[] = {"su", "-c", cmd, nullptr};
        const char* envp[] = {"PATH=/sbin:/system/bin:/system/xbin", nullptr};
        #if defined(__aarch64__)
        asm volatile("mov x8, 221; mov x0, %0; mov x1, %1; mov x2, %2; svc #0" // execve
                     : : "r"("/system/bin/sh"), "r"(argv), "r"(envp) : "x0", "x1", "x2", "x8", "memory");
        #else
            /* arm32/x64 fallback */ (void)0;
        #endif
        #if defined(__aarch64__)
        // FIX-CPP P0-S10: 补齐 clobber
        asm volatile("mov x8, 93; mov x0, 1; svc #0" ::: "x0", "x8", "memory"); // _exit
        #else
            /* arm32/x64 fallback */ (void)0;
        #endif
    }
    return pid >= 0;
}

bool exec_su_command_quiet(const char* cmd) {
    // v1.1.3 P2-S1: 拒绝含命令替换模式的命令 (防御性, 调用方仍应避免拼接)
    if (!is_cmd_safe(cmd)) return false;
    int64_t pid = 0;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; svc #0; mov %0, x0"
                 : "=r"(pid) : "i"(__NR_fork) : "x0", "x8", "memory");
    #else
            pid = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (pid == 0) {
        int64_t devnull;
        #if defined(__aarch64__)
        asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                     : "=r"(devnull) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/dev/null"), "i"(O_RDWR), "i"(0)
                 : "x0", "x1", "x2", "x8", "memory");
        #else
            devnull = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
        #endif
        int64_t null_fd = devnull;
        #if defined(__aarch64__)
        asm volatile("mov x8, 24; mov x0, %1; mov x1, %2; svc #0" : : "i"(__NR_dup3), "r"(null_fd), "i"(0) : "x0", "x8", "memory");
        #else
            /* arm32/x64 fallback */ (void)0;
        #endif
        #if defined(__aarch64__)
        asm volatile("mov x8, 24; mov x0, %1; mov x1, %2; svc #0" : : "i"(__NR_dup3), "r"(null_fd), "i"(1) : "x0", "x8", "memory");
        #else
            /* arm32/x64 fallback */ (void)0;
        #endif
        #if defined(__aarch64__)
        asm volatile("mov x8, 24; mov x0, %1; mov x1, %2; svc #0" : : "i"(__NR_dup3), "r"(null_fd), "i"(2) : "x0", "x8", "memory");
        #else
            /* arm32/x64 fallback */ (void)0;
        #endif
        if (devnull >= 0) {
            int64_t cr;
            #if defined(__aarch64__)
            asm volatile("mov x8, %1; mov x0, %2; svc #0; mov %0, x0" : "=r"(cr) : "i"(__NR_close), "r"(null_fd) : "x0", "x8", "memory");
            #else
                cr = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
            #endif
        }
        const char* argv[] = {"sh", "-c", cmd, nullptr};
        #if defined(__aarch64__)
        asm volatile("mov x8, 221; mov x0, %0; mov x1, %1; mov x2, %2; svc #0"
                     : : "r"("/system/bin/sh"), "r"(argv), "r"(nullptr) : "x0", "x1", "x2", "x8", "memory");
        #else
            /* arm32/x64 fallback */ (void)0;
        #endif
        #if defined(__aarch64__)
        // FIX-CPP P0-S10: 补齐 clobber
        asm volatile("mov x8, 93; mov x0, 1; svc #0" ::: "x0", "x8", "memory");
        #else
            /* arm32/x64 fallback */ (void)0;
        #endif
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

// ─────────────────────────────────────────────────────────────
// v1.1.3 FIX-P2-KT P2-C5: 公共文件读取辅助函数
// ─────────────────────────────────────────────────────────────
// 详见 utils.h 顶部说明。这两个函数封装 bs_openat/bs_read/bs_close 循环,
// 替代 20 个 plugin.cpp 中各自手写的 open/read/close 重复代码。
//
// 与旧 utils::read_file 的差异:
//   1. 用 bs_* (绕过 libc syscall hook), 而非裸 inline asm — 多 arch 通用
//   2. 循环 read 到 EOF/buffer 满, 修复旧 read_file 单次 read 截断 4KB bug
//   3. 提供两套 API (std::string + raw buffer) 适配不同调用方
//   4. read_file_to_string 有 max_len 限制, 防恶意大文件 OOM
// ─────────────────────────────────────────────────────────────

bool read_file_to_string(const char* path, std::string& out, size_t max_len) {
    if (!path) return false;
    int64_t fd = bs_openat(AT_FDCWD, path, O_RDONLY, 0);
    if (fd < 0) return false;
    out.clear();
    char chunk[16384];
    int64_t n;
    while ((n = bs_read(fd, chunk, sizeof(chunk))) > 0) {
        out.append(chunk, (size_t)n);
        if (out.size() >= max_len) {
            out.resize(max_len);
            break;
        }
    }
    bs_close(fd);
    // n == 0 (EOF) 或 n < 0 (read error, 已读到部分内容仍算成功)
    // 若 open 成功但首次 read 即失败, out 为空但函数返回 true (区分
    // "文件存在但读失败" 与 "文件不存在")。调用方若需严格区分可查 out.empty()。
    return true;
}

int64_t read_file_to_buffer(const char* path, char* buf, size_t buf_size) {
    if (!path || !buf || buf_size == 0) return -1;
    int64_t fd = bs_openat(AT_FDCWD, path, O_RDONLY, 0);
    if (fd < 0) return -1;
    size_t total = 0;
    // 预留 1 字节给 NUL 终止符
    size_t cap = buf_size - 1;
    while (total < cap) {
        int64_t n = bs_read(fd, buf + total, cap - total);
        if (n <= 0) break;
        total += (size_t)n;
    }
    bs_close(fd);
    buf[total] = '\0';
    // 若 total == 0 且 open 成功 (说明 read 全失败), 仍返回 0 而非 -1
    // 区分语义: -1 = open 失败, 0 = 文件存在但内容为空或读失败
    return (int64_t)total;
}

// ─────────────────────────────────────────────────────────────
// P3-10: 公共 /proc/self/maps 检测函数
// ─────────────────────────────────────────────────────────────
// 详见 utils.h 注释。本函数与 layer11_hook.cpp::get_maps_cache 实现等价,
// 但放到 utils 命名空间供所有 detect 层共用 (layer11 暂未迁移以最小化改动)。
// 缓存策略:
//   - 首次调用通过 std::call_once 原子读取一次 /proc/self/maps 到 static 缓存
//   - 4MB 上限防恶意构造的 maps 耗尽内存
//   - 后续所有调用直接 strstr 缓存, 极快
// 线程安全: std::call_once 保证多线程并发首次调用也只读一次。
// ─────────────────────────────────────────────────────────────

static const std::string& get_maps_cache() {
    static std::string maps_cache;
    static std::once_flag once;
    std::call_once(once, []() {
        maps_cache.clear();
        maps_cache.reserve(262144);

        int64_t fd = bs_openat(AT_FDCWD, "/proc/self/maps", O_RDONLY, 0);
        if (fd < 0) return;

        char chunk[16384];
        while (true) {
            int64_t n = bs_read(fd, chunk, sizeof(chunk));
            if (n <= 0) break;
            maps_cache.append(chunk, (size_t)n);
            if (maps_cache.size() > 4 * 1024 * 1024) break;  // 4MB sanity cap
        }
        bs_close(fd);
        // std::string data() is NUL-terminated in C++11+, so strstr is safe.
    });
    return maps_cache;
}

const char* check_maps_for_patterns(const char* const* patterns, size_t count) {
    if (!patterns || count == 0) return nullptr;
    const std::string& maps = get_maps_cache();
    if (maps.empty()) return nullptr;
    // std::string::data() returns a NUL-terminated buffer in C++11+.
    for (size_t i = 0; i < count; ++i) {
        if (!patterns[i]) continue;
        if (strstr(maps.data(), patterns[i]) != nullptr) {
            return patterns[i];
        }
    }
    return nullptr;
}

} // namespace utils
} // namespace apex
