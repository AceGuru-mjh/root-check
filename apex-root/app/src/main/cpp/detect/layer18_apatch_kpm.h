#ifndef APEX_ROOT_LAYER18_APATCH_KPM_H
#define APEX_ROOT_LAYER18_APATCH_KPM_H

#include <cstddef>

// ═══════════════════════════════════════════════════════════
//  第十八层 · APatch KPM 用户态检测 (root 级)
// ----------------------------------------------------------------
//  APatch 通过 KPM (Kernel Patch Module) 实现内核补丁,但其
//  用户态组件 (apd 守护进程、kpm 加载器、配置目录) 仍可被
//  root 级文件访问检测到。
//
//  本层覆盖:
//    - APatch KPM 模块目录 (/data/adb/ap/kpm/)
//    - APatch trampoline 后门特征 (kpmselinux / sigpatch)
//    - APatch 配置文件 (db, working, backup)
//    - APatch 与 KernelPatch 项目的关系文件
// ═══════════════════════════════════════════════════════════

#ifdef __cplusplus
extern "C" {
#endif

bool detectAPatchKPMUsermode(void);
bool detectAPatchTrampoline(void);
bool detectAPatchComponents(void);
bool detectKernelPatchProject(void);

int apatchKpmFullScan(char* out_report, size_t out_size);

#ifdef __cplusplus
}
#endif

#endif
