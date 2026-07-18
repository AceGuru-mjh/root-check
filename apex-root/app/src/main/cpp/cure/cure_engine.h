#ifndef APEX_ROOT_CURE_ENGINE_H
#define APEX_ROOT_CURE_ENGINE_H

#include <cstdint>

namespace apex {
namespace cure {

enum class CureLevel {
    LIGHT = 0,      // Remove obvious root traces
    STANDARD = 1,   // Fix 90% of detection issues
    DEEP = 2,       // Complete root residue removal
    FACTORY = 3     // Full factory reset
};

enum class RootType {
    MAGISK,
    KERNELSU,
    APATCH,
    UNKNOWN
};

struct CureResult {
    bool success;
    int items_removed;
    int items_failed;
    const char* message;
    bool needs_reboot;
};

// Auto-detect which root solution is installed
RootType detect_root_solution();

// Level 1: Remove obvious root traces
CureResult light_cleanup();

// Level 2: Standard fix - handles Magisk/KSU/APatch automatically
CureResult standard_fix(RootType root_type);

// Level 3: Deep recovery - remove all root residues  
CureResult deep_recovery();

// Level 4: Factory reset - full system restore
CureResult factory_reset();

// Backup and restore boot partition
bool backup_boot_partition(const char* backup_path);
bool restore_boot_partition(const char* backup_path);

// Verify cure is complete
bool verify_cure_complete();

// v1.1.1 修复 P0-D3: 用户确认标志 — 破坏性操作 (standard_fix/deep_recovery/
// factory_reset/restore_boot_partition) 必须先调用 set_destructive_confirmed(true)
// 才会执行。UI 层应在弹窗二次确认后设置此标志。
void set_destructive_confirmed(bool confirmed);
bool is_destructive_confirmed();

} // namespace cure
} // namespace apex

#endif
