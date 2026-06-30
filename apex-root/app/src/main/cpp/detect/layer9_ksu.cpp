#include "layer9_ksu.h"
#include "../common/syscall.h"
#include <cstring>

// ═══════════════════════════════════════════════════════════
//  第九层 · KernelSU 检测（root 级）
// ----------------------------------------------------------------
//  已移除 Ring0 检测：
//    - /proc/kernelsu          (KernelSU 内核 API 节点)
//    - /proc/modules 内扫描     (内核模块枚举)
//
//  现保留 / 扩充的均为 Ring3 root 级路径：
//    - /data/adb/ksu/*          (KernelSU 主目录)
//    - /data/adb/modules/ksu_*  (KernelSU 模块)
//    - /data/data/*manager*     (KernelSU Manager APP)
//    - /data/system/packages.xml 中 KernelSU Manager 包名
// ═══════════════════════════════════════════════════════════

static int64_t check_access(const char* path) {
    int64_t ret;
    asm volatile("mov x8, %1; mov x0, %2; mov x1, %3; svc #0; mov %0, x0"
                 : "=r"(ret) : "i"(__NR_access), "r"(path), "i"(F_OK) : "x0", "x1", "x8");
    return ret;
}

bool detectKernelSU() {
    // ─── 主目录 / 二进制 ────────────────────────────────────
    static const char* paths[] = {
        // 经典路径
        "/data/adb/ksu",
        "/data/adb/ksu/ksud",
        "/data/adb/ksu/bin",
        "/data/adb/ksu/bin/ksud",
        "/data/adb/ksu/conf",
        "/data/adb/ksu/log",
        "/data/adb/ksu/db",
        "/data/adb/ksu/db/kernel_su.db",
        "/data/adb/ksu/.allow_su",
        "/data/adb/ksu/.installed",
        "/data/adb/ksu/.config",
        "/data/adb/ksu/.update",
        // 模块目录
        "/data/adb/modules/kernelsu",
        "/data/adb/modules/ksu",
        "/data/adb/modules/KernelSU",
        // 兼容 SukiSU Ultra (KernelSU fork)
        "/data/adb/suki",
        "/data/adb/suki/sukid",
        "/data/adb/suki/ksud",
        "/data/adb/modules/sukisu",
        "/data/adb/modules/sukisu-ultra",
        // KernelSU NEXT (另一 fork)
        "/data/adb/ksu-next",
        "/data/adb/ksu-next/ksud",
        "/data/adb/modules/ksu-next",
        // 历史 db
        "/data/adb/ksu.db",
        // Magisk 模拟的 ksu 路径
        "/data/adb/modules/ksu_module",
        nullptr
    };
    for (auto p = paths; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }

    // ─── KernelSU Manager APP 包名检测 ────────────────────────
    // 这些包名在 /data/data 下可见（需 root）
    static const char* ksu_manager_packages[] = {
        "/data/data/me.weishu.kernelsu",
        "/data/data/me.weishu.kernelsu.nogui",
        "/data/data/com.kernelsu.manager",
        "/data/data/io.github.rifsxd.kernelsu",
        "/data/data/io.github.rifsxd.sukisu",
        "/data/data/io.github.rifsxd.sukisu.ultra",
        "/data/data/me.bmax.kernelsu",
        nullptr
    };
    for (auto p = ksu_manager_packages; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }

    return false;
}

bool detectKernelSUModule() {
    // 改为 root 级检测：扫描 /data/adb/modules 下的 KernelSU 模块
    // KernelSU 模块必含 module.prop 文件
    static const char* module_indicators[] = {
        "/data/adb/modules/kernelsu/module.prop",
        "/data/adb/modules/ksu/module.prop",
        "/data/adb/modules/KernelSU/module.prop",
        "/data/adb/modules/sukisu/module.prop",
        "/data/adb/modules/sukisu-ultra/module.prop",
        "/data/adb/modules/ksu-next/module.prop",
        // KernelSU AP 工具
        "/data/adb/ksu/ap",
        "/data/adb/ksu/bin/ap",
        nullptr
    };
    for (auto p = module_indicators; *p; ++p) {
        if (check_access(*p) == 0) return true;
    }
    return false;
}
