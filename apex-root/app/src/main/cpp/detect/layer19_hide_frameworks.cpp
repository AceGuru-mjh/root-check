#include "layer19_hide_frameworks.h"
#include "../common/syscall.h"
#include <cstring>
#include <cstdint>

static int64_t check_access(const char* path) {
    return apex_check_access(path);
}

// ─── Zygisk-Assistant (新型 Shamiko 替代) ─────────────────
bool detectZygiskAssistant() {
    static const char* paths[] = {
        "/data/adb/modules/zygisk_assistant",
        "/data/adb/modules/zygisk-assistant",
        "/data/adb/modules/zassistant",
        "/data/adb/modules/zygiskassistant",
        "/data/adb/zygisk_assistant",
        nullptr
    };
    for (auto p = paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

// ─── AML (Advanced Magisk Loader) ─────────────────────────
bool detectAmlLoader() {
    static const char* paths[] = {
        "/data/adb/aml",
        "/data/adb/modules/aml",
        "/data/adb/aml/bin",
        "/data/adb/aml/aml.sh",
        "/data/adb/modules/advanced_magisk_loader",
        nullptr
    };
    for (auto p = paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

// ─── MagiskFrida (Frida 服务端集成) ───────────────────────
bool detectMagiskFrida() {
    static const char* paths[] = {
        "/data/adb/modules/magisk_frida",
        "/data/adb/modules/magiskfrida",
        "/data/adb/modules/frida_server",
        "/data/adb/modules/frida-server",
        "/data/adb/magisk_frida",
        "/data/adb/frida",
        nullptr
    };
    for (auto p = paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

// ─── 持久化脚本检测 ──────────────────────────────────────
// Magisk / KSU / APatch 都支持 post-fs-data.d / service.d / boot-completed.d
// 持久化脚本。这些目录中的 .sh 文件是 root 方案的强信号。
bool detectPersistentScripts() {
    static const char* script_dirs[] = {
        // Magisk 持久化
        "/data/adb/service.d",
        "/data/adb/post-fs-data.d",
        "/data/adb/boot-completed.d",
        // KSU 持久化
        "/data/adb/ksu/service.d",
        "/data/adb/ksu/post-fs-data.d",
        "/data/adb/ksu/boot-completed.d",
        // APatch 持久化
        "/data/adb/ap/service.d",
        "/data/adb/ap/post-fs-data.d",
        "/data/adb/ap/boot-completed.d",
        // SukiSU 持久化
        "/data/adb/suki/service.d",
        "/data/adb/sukisu/service.d",
        // modules.d (部分 fork 使用)
        "/data/adb/modules.d",
        nullptr
    };
    for (auto p = script_dirs; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

// ─── 可疑模块检测 ─────────────────────────────────────────
// 扫描 /data/adb/modules/ 下的模块,查找已知隐藏/注入模块
bool detectSuspiciousModules() {
    static const char* suspicious_modules[] = {
        // 隐藏类
        "/data/adb/modules/shamiko",
        "/data/adb/modules/zygisknext",
        "/data/adb/modules/zygisk_next",
        "/data/adb/modules/zygisk-assistant",
        "/data/adb/modules/hidememapp",
        "/data/adb/modules/hidemyapplist",
        "/data/adb/modules/hma",
        "/data/adb/modules/storage_isolation",
        "/data/adb/modules/stiso",
        // 注入类
        "/data/adb/modules/zygisk_lsposed",
        "/data/adb/modules/zygisk-lsposed",
        "/data/adb/modules/lsposed",
        "/data/adb/modules/riru",
        "/data/adb/modules/riru_core",
        "/data/adb/modules/riru_lsposed",
        // Frida 集成
        "/data/adb/modules/magisk_frida",
        "/data/adb/modules/frida",
        // 字符串模糊匹配的别名
        "/data/adb/modules/.shamiko",
        "/data/adb/modules/.zygisknext",
        nullptr
    };
    for (auto p = suspicious_modules; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}

bool detectModernHideFrameworks() {
    return detectZygiskAssistant() ||
           detectAmlLoader() ||
           detectMagiskFrida() ||
           detectPersistentScripts() ||
           detectSuspiciousModules();
}

int hideFrameworksFullScan(char* out_report, size_t out_size) {
    if (!out_report || out_size == 0) return 0;
    int findings = 0;
    int pos = 0;
    auto append = [&](const char* s) {
        while (*s && pos < (int)out_size - 1) out_report[pos++] = *s++;
    };

    if (detectZygiskAssistant()) { append("[L19] Zygisk-Assistant detected\n"); findings++; }
    if (detectAmlLoader()) { append("[L19] AML (Advanced Magisk Loader) detected\n"); findings++; }
    if (detectMagiskFrida()) { append("[L19] MagiskFrida detected\n"); findings++; }
    if (detectPersistentScripts()) { append("[L19] Persistent root scripts detected\n"); findings++; }
    if (detectSuspiciousModules()) { append("[L19] Suspicious modules in /data/adb/modules detected\n"); findings++; }

    if (pos < (int)out_size) out_report[pos] = '\0';
    return findings;
}
