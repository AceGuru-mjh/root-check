#include "layer23_dhizuku.h"
#include "../common/syscall.h"
#include <cstring>

// ═══════════════════════════════════════════════════════════
//  第二十三层 · Dhizuku / 特权框架检测
// ═══════════════════════════════════════════════════════════

static int64_t check_access(const char* path) {
    return apex_check_access(path);
}

// ─── Dhizuku 检测 ─────────────────────────────────────────
bool detectDhizuku() {
    static const char* dhizuku_paths[] = {
        "/data/data/com.rosan.dhizuku",
        "/data/data/com.rosan.dhizuku.auto",
        "/data/user/0/com.rosan.dhizuku",
        nullptr
    };
    for (auto p = dhizuku_paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

// ─── Shizuku 检测 ─────────────────────────────────────────
bool detectShizuku() {
    static const char* shizuku_paths[] = {
        "/data/data/moe.shizuku.privileged.api",
        "/data/data/com.rikka.shizuku",
        "/data/user/0/com.rikka.shizuku",
        // Shizuku server 进程
        "/data/local/tmp/shizuku",
        "/data/local/tmp/rikka_shizuku_server",
        nullptr
    };
    for (auto p = shizuku_paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

// ─── Stellar 检测 (Shizuku fork) ──────────────────────────
bool detectStellar() {
    static const char* stellar_paths[] = {
        "/data/data/com.roro2239.stellar",
        "/data/data/stellar",
        "/data/local/tmp/stellar_server",
        nullptr
    };
    for (auto p = stellar_paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

// ─── 主检测入口 ───────────────────────────────────────────
bool detectPrivilegeFramework() {
    return detectDhizuku() || detectShizuku() || detectStellar();
}

int dhizukuFullScan(char* out_report, size_t out_size) {
    if (!out_report || out_size == 0) return 0;
    int findings = 0;
    int pos = 0;
    auto append = [&](const char* s) {
        while (*s && pos < (int)out_size - 1) out_report[pos++] = *s++;
    };

    if (detectDhizuku()) { append("[L23] Dhizuku (Device Owner privilege framework) detected\n"); findings++; }
    if (detectShizuku()) { append("[L23] Shizuku detected\n"); findings++; }
    if (detectStellar()) { append("[L23] Stellar (Shizuku fork) detected\n"); findings++; }

    if (pos < (int)out_size) out_report[pos] = '\0';
    return findings;
}
