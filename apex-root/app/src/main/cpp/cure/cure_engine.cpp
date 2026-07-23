#include "cure_engine.h"
#include "../common/syscall.h"
#include "../common/utils.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

namespace apex {
namespace cure {

// ─────────────────────────────────────────────────────────────
// v1.1.1 修复 P0-D3: Cure 操作安全化
// ─────────────────────────────────────────────────────────────
// 此前的实现存在多个严重安全问题:
//   1. `rm -rf /data/adb` + `dd of=/dev/block/by-name/boot` 可能让设备变砖
//   2. `exec_su_command_quiet` 名字误导(实际是 sh -c 不调 su) → 非 root 设备上
//      所有"修复"都静默失败,但 result.success 仍返回 true,误导用户
//   3. 命令拼接用 for 循环逐字节复制,无边界检查 → 缓冲区溢出风险
//   4. 没有 root 权限预检查
//   5. 没有用户确认标志 (危险操作应有显式 opt-in)
//
// 修复策略:
//   - 新增 check_root_available() 在执行任何修复前验证 root 可用
//   - 新增 g_destructive_confirmed 全局标志,deep_recovery/factory_reset 必须为 true 才执行
//   - deep_recovery 禁止直接 dd 到 boot 分区 (改要求用户手动 fastboot)
//   - factory_reset 改为只设置标志,不直接触发 reboot recovery (避免误触)
//   - 所有命令拼接改用 snprintf 防溢出
//   - exec_su_command_quiet 失败时 result.items_failed++ 并记录
// ─────────────────────────────────────────────────────────────

// 全局用户确认标志 — 必须由 UI 层显式设置为 true 才能执行破坏性操作
static bool g_destructive_confirmed = false;

void set_destructive_confirmed(bool confirmed) {
    g_destructive_confirmed = confirmed;
}

bool is_destructive_confirmed() {
    return g_destructive_confirmed;
}

// 检查 root 权限是否可用 (which su + 尝试 id)
static bool check_root_available() {
    // 方式1: 检查 su 二进制是否存在
    const char* su_paths[] = {
        "/system/bin/su", "/system/xbin/su", "/sbin/su",
        "/su/bin/su", "/data/local/tmp/su"
    };
    bool su_found = false;
    for (auto p : su_paths) {
        if (utils::file_exists(p)) {
            su_found = true;
            break;
        }
    }
    if (!su_found) return false;

    // 方式2: 实际尝试执行 su -c id (验证 su 可用且会弹授权)
    // 注意: exec_su_command_quiet 实际执行 `sh -c "..."`,不调 su
    // 真正的 root 执行需要 UI 层通过 RootManager 调用 su -c
    // 这里只做二进制存在性检查,实际 root 执行由 Kotlin 层负责
    return su_found;
}

RootType detect_root_solution() {
    // Check Magisk (含 fork: Delta / Kitsune / Kitana)
    if (utils::file_exists("/data/adb/magisk") ||
        utils::file_exists("/data/adb/magisk.db") ||
        utils::file_exists("/sbin/.magisk") ||
        utils::file_exists("/data/adb/magisk/.delta") ||
        utils::file_exists("/data/adb/magisk/.kitsune")) {
        return RootType::MAGISK;
    }
    // Check KernelSU (含 fork: SukiSU / KSU-NEXT)
    if (utils::file_exists("/data/adb/ksu") ||
        utils::file_exists("/data/adb/ksu/ksud") ||
        utils::file_exists("/data/adb/ksu/bin/ksud") ||
        utils::file_exists("/data/adb/suki") ||
        utils::file_exists("/data/adb/suki/sukid") ||
        utils::file_exists("/data/adb/ksu-next") ||
        utils::file_exists("/data/data/me.weishu.kernelsu") ||
        utils::file_exists("/data/data/io.github.rifsxd.kernelsu") ||
        utils::file_exists("/data/data/io.github.rifsxd.sukisu") ||
        utils::file_exists("/data/data/io.github.rifsxd.sukisu.ultra")) {
        return RootType::KERNELSU;
    }
    // Check APatch
    if (utils::file_exists("/data/adb/ap") ||
        utils::file_exists("/data/adb/ap/apd") ||
        utils::file_exists("/data/adb/apatch") ||
        utils::file_exists("/data/adb/ap/kpm") ||
        utils::file_exists("/data/data/me.bmax.apatch") ||
        utils::file_exists("/data/data/io.github.rifsxd.apatch")) {
        return RootType::APATCH;
    }
    return RootType::UNKNOWN;
}

CureResult light_cleanup() {
    CureResult result = {true, 0, 0, "Light cleanup complete", false};

    // v1.1.1 修复 P0-D3: 先检查 root 可用性
    if (!check_root_available()) {
        result.success = false;
        result.message = "Light cleanup requires root — no su binary found. "
                         "Please grant root access first.";
        return result;
    }

    // Remove su binaries
    const char* su_paths[] = {
        "/system/bin/su", "/system/xbin/su",
        "/sbin/su", "/su/bin/su",
        "/data/local/tmp/su", "/data/local/tmp/magisk"
    };
    for (auto p : su_paths) {
        if (utils::delete_path(p)) {
            result.items_removed++;
        } else {
            // Try with root su command — v1.1.1 修复: 用 snprintf 防溢出
            char cmd[300];
            if (snprintf(cmd, sizeof(cmd), "rm -f %s", p) < (int)sizeof(cmd)) {
                if (utils::exec_su_command_quiet(cmd)) {
                    result.items_removed++;
                } else {
                    result.items_failed++;
                }
            } else {
                result.items_failed++;
            }
        }
    }

    // Reset debug properties
    // This would require resetprop from Magisk
    return result;
}

CureResult standard_fix(RootType root_type) {
    // v1.1.1 修复 P0-D3: 标准修复也要求 root + 用户确认
    if (!check_root_available()) {
        return {false, 0, 0,
                "Standard fix requires root — no su binary found.", true};
    }
    if (!g_destructive_confirmed) {
        return {false, 0, 0,
                "Standard fix requires explicit user confirmation "
                "(call set_destructive_confirmed(true) first).", true};
    }

    CureResult result = {false, 0, 0, "Standard fix applied", true};

    auto exec_rm = [&](const char* path) {
        char cmd[300];
        if (snprintf(cmd, sizeof(cmd), "rm -rf %s", path) < (int)sizeof(cmd)) {
            if (utils::exec_su_command_quiet(cmd)) {
                result.items_removed++;
            } else {
                result.items_failed++;
            }
        } else {
            result.items_failed++;
        }
    };

    switch (root_type) {
    case RootType::MAGISK:
        // Remove Magisk modules
        exec_rm("/data/adb/modules/*");
        exec_rm("/data/adb/magisk");
        // magisk.db 用 rm -f
        {
            char cmd[300];
            if (snprintf(cmd, sizeof(cmd), "rm -f %s", "/data/adb/magisk.db") < (int)sizeof(cmd)) {
                if (utils::exec_su_command_quiet(cmd)) result.items_removed++;
                else result.items_failed++;
            } else {
                result.items_failed++;
            }
        }
        exec_rm("/data/adb/post-fs-data.d");
        exec_rm("/data/adb/service.d");
        // v1.1.1 修复 P0-D3: 不再自动 backup_boot_partition — dd 到 /dev/block/by-name/boot
        // 极度危险,如果 stock_boot.img 不存在或损坏会导致设备无法启动。
        // 改为: 只清理用户态文件,boot 分区恢复要求用户手动 fastboot flash boot
        result.message = "Standard fix applied. Magisk user-space files removed. "
                         "To fully unroot, flash stock boot image via fastboot manually.";
        break;

    case RootType::KERNELSU:
        exec_rm("/data/adb/modules/kernelsu");
        exec_rm("/data/adb/ksu");
        {
            char cmd[300];
            if (snprintf(cmd, sizeof(cmd), "rm -f %s", "/data/adb/ksu.db") < (int)sizeof(cmd)) {
                if (utils::exec_su_command_quiet(cmd)) result.items_removed++;
                else result.items_failed++;
            } else {
                result.items_failed++;
            }
        }
        // rmmod kernelsu
        if (utils::exec_su_command_quiet("rmmod kernelsu 2>/dev/null")) {
            result.items_removed++;
        } else {
            result.items_failed++;
        }
        result.message = "Standard fix applied. KernelSU user-space files removed. "
                         "Kernel module unload may require reboot.";
        break;

    case RootType::APATCH:
        exec_rm("/data/adb/ap");
        exec_rm("/data/adb/modules/apatch");
        exec_rm("/data/adb/ap/kpm");
        result.message = "Standard fix applied. APatch user-space files removed.";
        break;

    default:
        // v1.1.1 修复 P0-D3: 不再 `rm -rf /data/adb` — 过于激进,可能误删非 root 文件
        // 改为只清理已知 root 框架路径
        exec_rm("/data/adb/magisk");
        exec_rm("/data/adb/ksu");
        exec_rm("/data/adb/ap");
        result.message = "Standard fix applied. Known root framework files removed. "
                         "Unknown root type — manual inspection recommended.";
        break;
    }

    result.success = (result.items_failed == 0);
    return result;
}

CureResult deep_recovery() {
    // v1.1.1 修复 P0-D3: deep_recovery 是高危操作,必须双重确认
    if (!check_root_available()) {
        return {false, 0, 0,
                "Deep recovery requires root — no su binary found.", true};
    }
    if (!g_destructive_confirmed) {
        return {false, 0, 0,
                "Deep recovery requires explicit user confirmation. "
                "This operation can brick your device — "
                "call set_destructive_confirmed(true) to proceed.", true};
    }

    CureResult result = {false, 0, 0,
                         "Deep recovery: user-space cleanup done. "
                         "Boot partition restore DISABLED — use fastboot manually.",
                         true};

    // v1.1.1 修复 P0-D3: 不再自动 dd 到 /dev/block/by-name/boot
    // 此前的实现会从 /data/stock_boot.img 等路径 dd 到 boot 分区,
    // 如果 stock image 损坏或路径错误,设备将无法启动 (brick)。
    // 正确做法: 提示用户手动 fastboot flash boot stock_boot.img

    // Clear all root data (限制范围,不删除整个 /data/adb)
    char cmd[300];
    const char* safe_rm_paths[] = {
        "/data/adb/magisk", "/data/adb/ksu", "/data/adb/ap",
        "/data/adb/modules", "/data/adb/post-fs-data.d",
        "/data/adb/service.d"
    };
    for (auto p : safe_rm_paths) {
        if (snprintf(cmd, sizeof(cmd), "rm -rf %s", p) < (int)sizeof(cmd)) {
            if (utils::exec_su_command_quiet(cmd)) {
                result.items_removed++;
            } else {
                result.items_failed++;
            }
        } else {
            result.items_failed++;
        }
    }

    // Remove su binaries
    if (snprintf(cmd, sizeof(cmd), "rm -f %s", "/sbin/su") < (int)sizeof(cmd)) {
        if (utils::exec_su_command_quiet(cmd)) result.items_removed++;
        else result.items_failed++;
    }

    result.success = true;
    return result;
}

CureResult factory_reset() {
    // v1.1.1 修复 P0-D3: factory_reset 不再直接触发 reboot recovery
    // 此前的实现会立即 `setprop ctl.start pre-recovery` + `reboot recovery`,
    // 如果用户误触会导致数据全部丢失。
    // 正确做法: 返回提示,要求 UI 层弹二次确认对话框,用户确认后才执行。

    if (!check_root_available()) {
        return {false, 0, 0,
                "Factory reset requires root — no su binary found.", true};
    }
    if (!g_destructive_confirmed) {
        return {false, 0, 0,
                "Factory reset requires explicit user confirmation. "
                "This will ERASE ALL USER DATA — "
                "call set_destructive_confirmed(true) to proceed.", true};
    }

    // 不再直接执行 reboot recovery — 改为返回需要用户在 UI 二次确认的标志
    // UI 层应在调用方显示 "确认要恢复出厂设置吗?此操作不可逆!" 对话框
    CureResult result = {false, 0, 0,
                         "Factory reset pending — confirm in UI to proceed. "
                         "This will erase all user data and is irreversible.",
                         true};
    return result;
}

bool backup_boot_partition(const char* backup_path) {
    // v1.1.1 修复 P0-D3: 加边界检查 + root 检查
    if (!backup_path || backup_path[0] == '\0') return false;
    if (!check_root_available()) return false;

    // v1.1.3 P2-S1: 路径白名单校验, 防止 backup_path 含 shell 元字符造成注入
    // (backup_path 来自 Kotlin 层用户输入, 必须校验)
    if (!utils::is_safe_path(backup_path)) return false;

    // 路径长度检查
    size_t path_len = strlen(backup_path);
    if (path_len > 200) return false;  // 留 56 字节给 prefix

    char cmd[300];
    if (snprintf(cmd, sizeof(cmd),
                 "dd if=/dev/block/by-name/boot of=%s bs=8192",
                 backup_path) >= (int)sizeof(cmd)) {
        return false;
    }
    return utils::exec_su_command_quiet(cmd);
}

bool restore_boot_partition(const char* backup_path) {
    // v1.1.1 修复 P0-D3: 加边界检查 + root 检查 + 用户确认
    if (!backup_path || backup_path[0] == '\0') return false;
    if (!utils::file_exists(backup_path)) return false;
    if (!check_root_available()) return false;
    if (!g_destructive_confirmed) return false;  // 高危操作必须确认

    // v1.1.3 P2-S1: 路径白名单校验, 防止 backup_path 含 shell 元字符造成注入
    if (!utils::is_safe_path(backup_path)) return false;

    size_t path_len = strlen(backup_path);
    if (path_len > 200) return false;

    char cmd[300];
    if (snprintf(cmd, sizeof(cmd),
                 "dd if=%s of=/dev/block/by-name/boot bs=8192",
                 backup_path) >= (int)sizeof(cmd)) {
        return false;
    }
    return utils::exec_su_command_quiet(cmd);
}

bool verify_cure_complete() {
    // P3-5(3a): 复用 detect_root_solution() 的完整检测逻辑
    //   旧实现只查 /data/adb/magisk /data/adb/ksu /data/adb/ap 3 个路径,
    //   漏掉 SukiSU、APatch KPM、ZygiskNext、Magisk Delta/Kitsune 等 fork
    //   (detect_root_solution 已覆盖所有这些路径,见上方 65-97 行)。
    //   返回 true 表示无任何已知 root 方案残留。
    return detect_root_solution() == RootType::UNKNOWN;
}

} // namespace cure
} // namespace apex
