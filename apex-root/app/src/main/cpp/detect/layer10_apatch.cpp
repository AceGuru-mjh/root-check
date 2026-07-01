#include "layer10_apatch.h"
#include "../common/syscall.h"
#include <cstring>

// ═══════════════════════════════════════════════════════════
//  第十层 · APatch 检测（root 级）
// ----------------------------------------------------------------
//  已移除 Ring0 检测：
//    - /sys/kpm                (KPM 内核 sysfs 节点)
//    - /proc/kallsyms          (内核符号表扫描，kptr_restrict 屏蔽)
//
//  现保留 / 扩充的均为 Ring3 root 级路径：
//    - /data/adb/ap/*          (APatch 主目录)
//    - /data/adb/modules/ap*   (APatch 模块)
//    - /data/data/*manager*    (APatch Manager APP)
//    - KPM 用户态模块目录
// ═══════════════════════════════════════════════════════════

static int64_t check_access(const char* path) {
    int64_t ret;
    ret = apex_check_access(path);
    return ret;
}

bool detectAPatch() {
    // ─── APatch 主目录 / 二进制 ───────────────────────────────
    static const char* paths[] = {
        "/data/adb/ap",
        "/data/adb/ap/apd",
        "/data/adb/ap/working",
        "/data/adb/ap/backup",
        "/data/adb/ap/bin",
        "/data/adb/ap/bin/apd",
        "/data/adb/ap/db",
        "/data/adb/ap/db/apatch.db",
        "/data/adb/ap/conf",
        "/data/adb/ap/log",
        "/data/adb/ap/.installed",
        "/data/adb/ap/.config",
        "/data/adb/apatch",
        "/data/adb/apatch/apd",
        // 模块目录
        "/data/adb/modules/apatch",
        "/data/adb/modules/APatch",
        "/data/adb/modules/ap",
        nullptr
    };
    for (auto p = paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }

    // ─── APatch Manager APP 包名 ─────────────────────────────
    static const char* apatch_manager_packages[] = {
        "/data/data/me.bmax.apatch",
        "/data/data/me.bmax.apatch.nogui",
        "/data/data/com.apatch.manager",
        "/data/data/io.github.rifsxd.apatch",
        nullptr
    };
    for (auto p = apatch_manager_packages; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }

    return false;
}

bool detectAPatchKPM() {
    // 改为 root 级检测：KPM 用户态模块目录
    // KPM (Kernel Patch Module) 在用户态会通过 /data/adb/ap/kpm 管理
    static const char* kpm_indicators[] = {
        // KPM 模块存放目录
        "/data/adb/ap/kpm",
        "/data/adb/ap/kpm/",
        "/data/adb/ap/bin/kpm",
        // KPM 模块管理脚本
        "/data/adb/ap/kpm_loader",
        "/data/adb/ap/.kpm_installed",
        // KPM 日志
        "/data/adb/ap/log/kpm.log",
        // APatch 模块 prop
        "/data/adb/modules/apatch/module.prop",
        "/data/adb/modules/APatch/module.prop",
        // KPM 自动加载标记
        "/data/adb/ap/kpm/autostart",
        nullptr
    };
    for (auto p = kpm_indicators; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}
