#ifndef APEX_ROOT_LAYER19_HIDE_FRAMEWORKS_H
#define APEX_ROOT_LAYER19_HIDE_FRAMEWORKS_H

#include <cstddef>

// ═══════════════════════════════════════════════════════════
//  第十九层 · 隐藏框架深度检测 (root 级)
// ----------------------------------------------------------------
//  现代 root 隐藏方案不止 Shamiko/ZygiskNext,还有:
//    - Zygisk-Assistant (新型 Shamiko 替代,2024 出现)
//    - AML (Advanced Magisk Loader)
//    - MagiskFrida (Frida 服务端集成到 Magisk)
//    - Various modules under /data/adb/modules/*/
//    - post-fs-data.d / service.d 持久化脚本
//    - boot-completed.d 启动完成钩子
//
//  本层扫描模块目录结构 + 持久化脚本,即使模块本身被卸载,
//  残留的脚本仍可能被检测到。
// ═══════════════════════════════════════════════════════════

#ifdef __cplusplus
extern "C" {
#endif

bool detectZygiskAssistant(void);
bool detectAmlLoader(void);
bool detectMagiskFrida(void);
bool detectPersistentScripts(void);
bool detectSuspiciousModules(void);
bool detectModernHideFrameworks(void);

int hideFrameworksFullScan(char* out_report, size_t out_size);

#ifdef __cplusplus
}
#endif

#endif
