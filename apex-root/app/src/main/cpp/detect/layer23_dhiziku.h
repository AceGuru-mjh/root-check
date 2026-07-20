#ifndef APEX_ROOT_LAYER23_DHIZUKU_H
#define APEX_ROOT_LAYER23_DHIZUKU_H

#include <cstddef>

// ═══════════════════════════════════════════════════════════
//  第二十三层 · Dhizuku / 设备所有者特权框架检测 (root 级)
// ----------------------------------------------------------------
//  Dhizuku 是 Shizuku 的替代,利用 Android 设备所有者
//  (Device Owner) 权限提供系统级 API 访问。无需 root 或 ADB。
//  与 Shizuku 不同,Dhizuku 通过 DeviceAdmin/DeviceOwner 权限工作。
//
//  同时检测其他非 root 特权框架:
//    - Shizuku (rikkaapps)
//    - Stellar (roro2239 Shizuku fork)
//    - AppOps (RikkaApps)
// ═══════════════════════════════════════════════════════════

#ifdef __cplusplus
extern "C" {
#endif

bool detectDhizuku(void);
bool detectShizuku(void);
bool detectStellar(void);
bool detectPrivilegeFramework(void);

int dhizukuFullScan(char* out_report, size_t out_size);

#ifdef __cplusplus
}
#endif

#endif
