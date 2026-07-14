#ifndef APEX_ROOT_LAYER6_KERNEL_H
#define APEX_ROOT_LAYER6_KERNEL_H

// ═══════════════════════════════════════════════════════════
//  第六层（原内核完整性，现 Root 守护进程检测）
// ----------------------------------------------------------------
//  原 Ring0 检测（/proc/kallsyms, /proc/modules, kernel taint）
//  已移除，改为 root 级守护进程 / service 脚本 / 配置 DB 检测。
//
//  函数名保持兼容（避免改动 JNI 调用链），但语义已变：
//    - detectKernelTampering()       → root 守护进程扫描
//    - detectKernelModuleTampering() → root 方案 service 目录
//    - detectKernelTaint()           → root 方案配置 DB
// ═══════════════════════════════════════════════════════════

bool detectKernelTampering();          // root daemon 进程扫描
bool detectKernelModuleTampering();    // service / post-fs-data 脚本
bool detectKernelTaint();              // root 方案 DB / 配置文件

#endif // APEX_ROOT_LAYER6_KERNEL_H
