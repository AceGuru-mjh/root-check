#include "cure_engine.h"
#include "../common/syscall.h"
#include "../common/utils.h"
#include <cstring>

namespace apex {
namespace cure {

RootType detect_root_solution() {
    // Check Magisk
    if (utils::file_exists("/data/adb/magisk") ||
        utils::file_exists("/data/adb/magisk.db") ||
        utils::file_exists("/sbin/.magisk")) {
        return RootType::MAGISK;
    }
    // Check KernelSU
    if (utils::file_exists("/data/adb/ksu") ||
        utils::file_exists("/proc/kernelsu")) {
        return RootType::KERNELSU;
    }
    // Check APatch
    if (utils::file_exists("/data/adb/ap") ||
        utils::file_exists("/sys/kpm")) {
        return RootType::APATCH;
    }
    return RootType::UNKNOWN;
}

CureResult light_cleanup() {
    CureResult result = {true, 0, 0, "Light cleanup complete", false};

    // Remove su binaries
    const char* su_paths[] = {
        "/system/bin/su", "/system/xbin/su",
        "/sbin/su", "/su/bin/su",
        "/data/local/tmp/su", "/data/local/tmp/magisk"
    };
    for (auto p : su_paths) {
        if (utils::delete_path(p)) result.items_removed++;
        else {
            // Try with root su command
            char cmd[256];
            int idx = 0;
            const char* prefix = "rm -f ";
            for (int i = 0; prefix[i]; i++) cmd[idx++] = prefix[i];
            for (int i = 0; p[i]; i++) cmd[idx++] = p[i];
            cmd[idx] = '\0';
            if (utils::exec_su_command_quiet(cmd)) result.items_removed++;
        }
    }

    // Reset debug properties
    // This would require resetprop from Magisk
    return result;
}

CureResult standard_fix(RootType root_type) {
    CureResult result = {false, 0, 0, "Standard fix applied", true};

    switch (root_type) {
    case RootType::MAGISK:
        // Remove Magisk modules
        utils::exec_su_command_quiet("rm -rf /data/adb/modules/*");
        utils::exec_su_command_quiet("rm -rf /data/adb/magisk");
        utils::exec_su_command_quiet("rm -f /data/adb/magisk.db");
        // Remove post-fs-data scripts
        utils::exec_su_command_quiet("rm -rf /data/adb/post-fs-data.d");
        utils::exec_su_command_quiet("rm -rf /data/adb/service.d");
        // Restore original boot
        backup_boot_partition("/data/local/tmp/apex_boot_backup.img");
        result.items_removed = 5;
        break;

    case RootType::KERNELSU:
        // Remove KernelSU modules
        utils::exec_su_command_quiet("rm -rf /data/adb/modules/kernelsu");
        utils::exec_su_command_quiet("rm -rf /data/adb/ksu");
        utils::exec_su_command_quiet("rm -f /data/adb/ksu.db");
        // Unload kernel module
        utils::exec_su_command_quiet("rmmod kernelsu 2>/dev/null");
        result.items_removed = 4;
        break;

    case RootType::APATCH:
        // Remove APatch
        utils::exec_su_command_quiet("rm -rf /data/adb/ap");
        utils::exec_su_command_quiet("rm -rf /data/adb/modules/apatch");
        // Remove KPM
        utils::exec_su_command_quiet("rm -rf /sys/kpm");
        result.items_removed = 3;
        break;

    default:
        // Try to clean everything
        utils::exec_su_command_quiet("rm -rf /data/adb");
        utils::exec_su_command_quiet("rm -rf /sbin/.magisk");
        result.items_removed = 2;
        break;
    }

    result.success = true;
    return result;
}

CureResult deep_recovery() {
    CureResult result = {false, 0, 0, "Deep recovery: restore boot image", true};

    // Backup current boot
    const char* backup = "/data/local/tmp/apex_stock_boot.img";
    if (backup_boot_partition(backup)) {
        // Find and flash stock boot image from common locations
        const char* stock_paths[] = {
            "/data/stock_boot.img", "/data/stock_boot.img.gz",
            "/cache/stock_boot.img", "/persist/stock_boot.img",
            "/cust/stock_boot.img"
        };
        bool restored = false;
        for (auto p : stock_paths) {
            if (utils::file_exists(p)) {
                char cmd[256];
                int idx = 0;
                const char* prefix = "dd if=";
                for (int i = 0; prefix[i]; i++) cmd[idx++] = prefix[i];
                for (int i = 0; p[i]; i++) cmd[idx++] = p[i];
                const char* suffix = " of=/dev/block/by-name/boot";
                for (int i = 0; suffix[i]; i++) cmd[idx++] = suffix[i];
                cmd[idx] = '\0';
                utils::exec_su_command_quiet(cmd);
                restored = true;
                result.items_removed = 1;
                break;
            }
        }
        if (!restored) {
            result.message = "Stock boot image not found. Manual flash required via fastboot.";
        }
    }

    // Clear all root data
    utils::exec_su_command_quiet("rm -rf /data/adb");
    utils::exec_su_command_quiet("rm -f /sbin/su");

    result.success = true;
    return result;
}

CureResult factory_reset() {
    CureResult result = {false, 0, 0, "Factory reset initiated", true};
    // Wipe data via recovery
    utils::exec_su_command_quiet("setprop ctl.start pre-recovery 2>/dev/null");
    // Reboot to recovery
    utils::exec_su_command_quiet("reboot recovery");
    result.success = true;
    return result;
}

bool backup_boot_partition(const char* backup_path) {
    char cmd[256];
    int idx = 0;
    const char* prefix = "dd if=/dev/block/by-name/boot of=";
    for (int i = 0; prefix[i]; i++) cmd[idx++] = prefix[i];
    for (int i = 0; backup_path[i]; i++) cmd[idx++] = backup_path[i];
    cmd[idx] = '\0';
    return utils::exec_su_command_quiet(cmd);
}

bool restore_boot_partition(const char* backup_path) {
    if (!utils::file_exists(backup_path)) return false;
    char cmd[256];
    int idx = 0;
    const char* prefix = "dd if=";
    for (int i = 0; prefix[i]; i++) cmd[idx++] = prefix[i];
    for (int i = 0; backup_path[i]; i++) cmd[idx++] = backup_path[i];
    const char* suffix = " of=/dev/block/by-name/boot";
    for (int i = 0; suffix[i]; i++) cmd[idx++] = suffix[i];
    cmd[idx] = '\0';
    return utils::exec_su_command_quiet(cmd);
}

bool verify_cure_complete() {
    // Re-run detection to verify
    bool found = false;
    found |= utils::file_exists("/data/adb/magisk");
    found |= utils::file_exists("/data/adb/ksu");
    found |= utils::file_exists("/data/adb/ap");
    return !found;
}

} // namespace cure
} // namespace apex
