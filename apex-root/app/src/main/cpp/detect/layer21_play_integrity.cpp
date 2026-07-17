#include "layer21_play_integrity.h"
#include "../common/syscall.h"
#include <cstring>
#include <cstdint>

// ═══════════════════════════════════════════════════════════
//  第二十一层 · Play Integrity API 检测 (root 级)
// ═══════════════════════════════════════════════════════════

static int64_t check_access(const char* path) {
    return apex_check_access(path);
}

// ─── PIF 模块检测 ─────────────────────────────────────────
// PlayIntegrityFix 是 Magisk/KernelSU 模块,伪造 Play Integrity 判定
bool detectPIFModule() {
    static const char* pif_paths[] = {
        "/data/adb/modules/playintegrityfix",
        "/data/adb/modules/PlayIntegrityFix",
        "/data/adb/modules/pif",
        "/data/adb/modules/pif_next",
        "/data/adb/pif.json",
        "/data/adb/modules/playintegrityfix/pif.json",
        "/data/adb/modules/pif/pif.json",
        // PIF Fork 变体
        "/data/adb/modules/pifork",
        "/data/adb/modules/PIFork",
        nullptr
    };
    for (auto p = pif_paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

// ─── TrickyStore 检测 ──────────────────────────────────────
// TrickyStore 伪造硬件证明密钥,绕过 STRONG_INTEGRITY
bool detectTrickyStoreModule() {
    static const char* ts_paths[] = {
        "/data/adb/tricky_store",
        "/data/adb/modules/tricky_store",
        "/data/adb/modules/TrickyStore",
        "/data/adb/tricky_store/targets",
        "/data/adb/tricky_store/keystore",
        nullptr
    };
    for (auto p = ts_paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

// ─── Google Play Services 可用性检测 ──────────────────────
bool detectGmsAvailable() {
    // 检查 GMS 核心包
    static const char* gms_paths[] = {
        "/data/data/com.google.android.gms",
        "/data/data/com.google.android.gsf",
        "/data/data/com.android.vending",
        "/system/app/PrebuiltGmsCore",
        "/system/priv-app/PrebuiltGmsCore",
        "/product/app/PrebuiltGmsCore",
        nullptr
    };
    for (auto p = gms_paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

// ─── 主检测入口 ───────────────────────────────────────────
bool detectPlayIntegrityTampering() {
    return detectPIFModule() || detectTrickyStoreModule();
}

int playIntegrityFullScan(char* out_report, size_t out_size) {
    if (!out_report || out_size == 0) return 0;
    int findings = 0;
    int pos = 0;
    auto append = [&](const char* s) {
        while (*s && pos < (int)out_size - 1) out_report[pos++] = *s++;
    };

    if (detectPIFModule()) { append("[L21] PlayIntegrityFix module detected\n"); findings++; }
    if (detectTrickyStoreModule()) { append("[L21] TrickyStore module detected\n"); findings++; }
    if (!detectGmsAvailable()) { append("[L21] Google Play Services not found (Play Integrity unavailable)\n"); findings++; }

    if (pos < (int)out_size) out_report[pos] = '\0';
    return findings;
}
