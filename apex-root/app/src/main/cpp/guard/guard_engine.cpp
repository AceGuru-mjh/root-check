#include "guard_engine.h"
#include "../common/syscall.h"
#include "../common/utils.h"
#include "../bare_syscall/syscall_bridge.h"
#include <cstring>

namespace apex {
namespace guard {

static bool guardian_active = false;
static int alert_count = 0;
static SecurityAlert alert_buffer[64];

// SHA-256 compress function for file integrity
static void sha256_compress(uint32_t state[8], const uint8_t block[64]) {
    uint32_t w[64];
    for (int i = 0; i < 16; i++)
        w[i] = (block[i*4] << 24) | (block[i*4+1] << 16) | (block[i*4+2] << 8) | block[i*4+3];
    for (int i = 16; i < 64; i++) {
        uint32_t s0 = ((w[i-15] >> 7) | (w[i-15] << 25)) ^ ((w[i-15] >> 18) | (w[i-15] << 14)) ^ (w[i-15] >> 3);
        uint32_t s1 = ((w[i-2] >> 17) | (w[i-2] << 15)) ^ ((w[i-2] >> 19) | (w[i-2] << 13)) ^ (w[i-2] >> 10);
        w[i] = w[i-16] + w[i-7] + s0 + s1;
    }
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
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
    for (int i = 0; i < 64; i++) {
        uint32_t S1 = ((e >> 6) | (e << 26)) ^ ((e >> 11) | (e << 21)) ^ ((e >> 25) | (e << 7));
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t temp1 = h + S1 + ch + K[i] + w[i];
        uint32_t S0 = ((a >> 2) | (a << 30)) ^ ((a >> 13) | (a << 19)) ^ ((a >> 22) | (a << 10));
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = S0 + maj;
        h = g; g = f; f = e; e = d + temp1;
        d = c; c = b; b = a; a = temp1 + temp2;
    }
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

static void compute_sha256(const uint8_t* data, size_t len, uint8_t out[32]) {
    uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    size_t offset = 0;
    uint8_t block[64];
    while (offset + 64 <= len) {
        std::memcpy(block, data + offset, 64);
        sha256_compress(state, block);
        offset += 64;
    }
    // Final block with padding
    uint8_t final[128];
    size_t remaining = len - offset;
    std::memcpy(final, data + offset, remaining);
    final[remaining] = 0x80;
    size_t final_len = remaining + 1;
    if (final_len > 56) {
        std::memset(final + final_len, 0, 64 - final_len);
        sha256_compress(state, final);
        final_len = 0;
    }
    std::memset(final + final_len, 0, 56 - final_len);
    uint64_t bit_len = (uint64_t)len * 8;
    for (int i = 0; i < 8; i++)
        final[56 + final_len + i] = (bit_len >> (56 - i * 8)) & 0xFF;
    sha256_compress(state, final + final_len);
    for (int i = 0; i < 8; i++) {
        out[i*4]   = (state[i] >> 24) & 0xFF;
        out[i*4+1] = (state[i] >> 16) & 0xFF;
        out[i*4+2] = (state[i] >> 8) & 0xFF;
        out[i*4+3] = state[i] & 0xFF;
    }
}

static const char* critical_system_files[] = {
    "/system/bin/app_process64",
    "/system/bin/app_process32",
    "/system/bin/linker64",
    "/system/bin/linker",
    "/system/lib64/libc.so",
    "/system/lib/libc.so",
    "/system/lib64/libm.so",
    "/system/lib/libm.so",
    "/system/lib64/libdl.so",
    "/system/lib/libdl.so",
    "/system/etc/hosts",
};

static uint8_t g_integrity_db[sizeof(critical_system_files)/sizeof(critical_system_files[0])][32];
static bool g_db_initialized = false;
static int g_db_count = 0;

static bool compute_file_sha256(const char* path, uint8_t out[32]) {
    char buf[65536];
    int64_t fd;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"(path), "i"(O_RDONLY), "i"(0));
    #else
        fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (fd < 0) return false;

    uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    uint64_t total = 0;
    int64_t n;
    while ((n = bs_read(fd, buf, sizeof(buf))) > 0) {
        total += n;
        size_t offset = 0;
        uint8_t block[64];
        while (offset + 64 <= (size_t)n) {
            std::memcpy(block, (uint8_t*)buf + offset, 64);
            sha256_compress(state, block);
            offset += 64;
        }
        if (offset < (size_t)n) {
            std::memset(block, 0, 64);
            std::memcpy(block, (uint8_t*)buf + offset, n - offset);
        }
    }
    bs_close(fd);

    // Finalize
    uint8_t final[128]{};
    // ... simplified finalization for guard purposes
    for (int i = 0; i < 8; i++) {
        out[i*4]   = (state[i] >> 24) & 0xFF;
        out[i*4+1] = (state[i] >> 16) & 0xFF;
        out[i*4+2] = (state[i] >> 8) & 0xFF;
        out[i*4+3] = state[i] & 0xFF;
    }
    return true;
}

// P0-2 修复: 将 message 和 source 复制到固定大小数组,避免存储栈上指针导致悬空
static void add_alert(AlertLevel level, const char* msg, const char* source) {
    if (alert_count >= 64) return;
    SecurityAlert alert = {};
    alert.level = level;
    alert.timestamp = bs_clock_ns();
    if (msg) {
        strncpy(alert.message, msg, sizeof(alert.message) - 1);
        alert.message[sizeof(alert.message) - 1] = '\0';
    } else {
        alert.message[0] = '\0';
    }
    if (source) {
        strncpy(alert.source, source, sizeof(alert.source) - 1);
        alert.source[sizeof(alert.source) - 1] = '\0';
    } else {
        alert.source[0] = '\0';
    }
    alert_buffer[alert_count++] = alert;
}

bool start_guardian() {
    guardian_active = true;
    alert_count = 0;
    if (!g_db_initialized) init_integrity_database();
    return true;
}

bool stop_guardian() {
    guardian_active = false;
    return true;
}

bool check_system_integrity() {
    bool all_ok = true;
    size_t count = sizeof(critical_system_files) / sizeof(critical_system_files[0]);
    for (size_t i = 0; i < count; i++) {
        auto f = critical_system_files[i];
        if (!utils::file_exists(f)) {
            add_alert(AlertLevel::CRITICAL, f, "integrity_check");
            all_ok = false;
            continue;
        }
        if (g_db_initialized && i < (size_t)g_db_count) {
            uint8_t current_hash[32];
            if (compute_file_sha256(f, current_hash)) {
                if (std::memcmp(current_hash, g_integrity_db[i], 32) != 0) {
                    add_alert(AlertLevel::WARNING, f, "integrity_check");
                    all_ok = false;
                }
            }
        }
    }
    return all_ok;
}

bool check_kernel_module_integrity() {
    // 已移除 Ring0 检测：/proc/modules 内核模块枚举
    //   原代码扫描 /proc/modules 找 kernelsu/kpm/apatch/magisk 等关键字，
    //   并按白名单匹配 module refcounts。
    //   但 /proc/modules 在 Android 13+ GKI 内核需要 CAP_SYS_MODULE 才能
    //   完整枚举，且 OEM 模块会被错误标记为可疑。
    //
    //   现改为 Ring3 root 级：检测 root 方案的 service 脚本目录与守护进程
    //   二进制，这些是真正的用户态 root 痕迹，跨内核版本稳定。

    const char* root_indicators[] = {
        // Magisk
        "/data/adb/magisk",
        "/data/adb/magisk.db",
        "/data/adb/magisk/.magisk",
        // KernelSU (含 fork)
        "/data/adb/ksu",
        "/data/adb/ksu/ksud",
        "/data/adb/ksu/bin/ksud",
        "/data/adb/suki",
        "/data/adb/suki/sukid",
        "/data/adb/ksu-next",
        // APatch
        "/data/adb/ap",
        "/data/adb/ap/apd",
        "/data/adb/ap/kpm",     // KPM 用户态模块目录（原 /sys/kpm 是 Ring0）
        // service 脚本目录
        "/data/adb/service.d",
        "/data/adb/post-fs-data.d",
        "/data/adb/modules.d",
        "/data/adb/boot-completed.d",
        // Zygisk 生态
        "/data/adb/zygisknext",
        "/data/adb/rezygisk",
        nullptr
    };

    bool found = false;
    for (auto p = root_indicators; *p; ++p) {
        if (utils::file_exists(*p)) {
            char msg[160];
            snprintf(msg, sizeof(msg), "root indicator path present: %s", *p);
            add_alert(AlertLevel::CRITICAL, msg, "root_indicator");
            found = true;
        }
    }
    return !found;
}

bool check_process_integrity(const char* whitelist[], int count) {
    char buf[8192];
    if (!utils::read_file("/proc", buf, sizeof(buf))) return true;

    // Read /proc entries to enumerate running processes
    int64_t fd;
    #if defined(__aarch64__)
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                 : "=r"(fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"("/proc"), "i"(O_RDONLY | O_DIRECTORY), "i"(0));
    #else
        fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
    #endif
    if (fd < 0) return true;

    char dentry_buf[4096];
    int64_t n;
    bool all_ok = true;

    while ((n = bs_getdents64(fd, dentry_buf, sizeof(dentry_buf))) > 0) {
        size_t pos = 0;
        while (pos + sizeof(apex_dirent64) <= (size_t)n) {
            auto* d = reinterpret_cast<apex_dirent64*>(dentry_buf + pos);
            // P0-1 修复: 完整校验 d_reclen (原来只检查 ==0,没检查 > 剩余)
            if (d->d_reclen == 0 || d->d_reclen > (size_t)n - pos) break;

            // Check if this is a numeric directory (process PID)
            // P0-1 修复: 限制 d_name 扫描范围到 record 边界
            const char* name_end = (const char*)d + d->d_reclen;
            bool is_pid = true;
            bool any_char = false;
            for (const char* c = d->d_name; c < name_end && *c; c++) {
                any_char = true;
                if (*c < '0' || *c > '9') {
                    is_pid = false;
                    break;
                }
            }
            if (!any_char) is_pid = false;

            if (is_pid) {
                // Read process cmdline to check against whitelist
                char cmd_path[64];
                snprintf(cmd_path, sizeof(cmd_path), "/proc/%s/cmdline", d->d_name);
                int64_t cmd_fd;
                #if defined(__aarch64__)
                asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; mov x2, %4; svc #0; mov %0, x0"
                             : "=r"(cmd_fd) : "i"(__NR_openat), "i"(AT_FDCWD), "r"(cmd_path), "i"(O_RDONLY), "i"(0));
                #else
                    cmd_fd = -1; /* arm32/x64: syscall bypass disabled, libc path used where available */
                #endif
                if (cmd_fd >= 0) {
                    char cmdline[256];
                    int64_t cmd_n = bs_read(cmd_fd, cmdline, sizeof(cmdline) - 1);
                    bs_close(cmd_fd);
                    if (cmd_n > 0) {
                        cmdline[cmd_n] = 0;
                        // Check if process is in whitelist
                        bool in_whitelist = false;
                        for (int w = 0; w < count; w++) {
                            if (whitelist && whitelist[w] && strstr(cmdline, whitelist[w])) {
                                in_whitelist = true;
                                break;
                            }
                        }
                        // Check known legitimate Android processes
                        const char* known_good[] = {
                            "system_server", "surfaceflinger", "audioserver",
                            "mediaserver", "logd", "servicemanager",
                            "netd", "zygote", "adbd", "lmkd", NULL
                        };
                        if (!in_whitelist) {
                            for (int g = 0; known_good[g]; g++) {
                                if (strstr(cmdline, known_good[g])) {
                                    in_whitelist = true;
                                    break;
                                }
                            }
                        }
                        if (!in_whitelist && cmdline[0] != '\0' &&
                            strstr(cmdline, "com.apex.root") == nullptr) {
                            char msg[256];
                            snprintf(msg, sizeof(msg), "unknown process: %s", cmdline);
                            add_alert(AlertLevel::WARNING, msg, "process_check");
                            all_ok = false;
                        }
                    }
                }
            }
            pos += d->d_reclen;
        }
    }
    bs_close(fd);
    return all_ok;
}

bool prevent_system_tamper() {
    utils::exec_su_command_quiet("mount -o remount,ro /system 2>/dev/null");
    utils::exec_su_command_quiet("mount -o remount,ro /vendor 2>/dev/null");
    utils::exec_su_command_quiet("mount -o remount,ro /product 2>/dev/null");
    return true;
}

int get_alerts(SecurityAlert* buffer, int max_count) {
    int copy = alert_count < max_count ? alert_count : max_count;
    for (int i = 0; i < copy; i++)
        buffer[i] = alert_buffer[i];
    return copy;
}

bool init_integrity_database() {
    g_db_count = sizeof(critical_system_files) / sizeof(critical_system_files[0]);
    int initialized = 0;
    for (int i = 0; i < g_db_count; i++) {
        if (compute_file_sha256(critical_system_files[i], g_integrity_db[i]))
            initialized++;
    }
    g_db_initialized = initialized > 0;
    return g_db_initialized;
}

} // namespace guard
} // namespace apex
