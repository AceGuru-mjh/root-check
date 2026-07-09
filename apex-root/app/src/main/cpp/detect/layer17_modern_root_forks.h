#ifndef APEX_ROOT_LAYER17_MODERN_ROOT_FORKS_H
#define APEX_ROOT_LAYER17_MODERN_ROOT_FORKS_H

#include <cstddef>

// ═══════════════════════════════════════════════════════════
//  第十七层 · 现代 Root 方案 Fork 检测 (root 级)
// ----------------------------------------------------------------
//  2024-2025 root 生态出现大量 fork:
//    - SukiSU / SukiSU-Ultra (KernelSU fork,增强隐藏)
//    - ReZygisk (Rust 实现的 Zygisk 替代)
//    - ZygiskNext variants (NextDelta 等)
//    - Magisk Delta / Kitsune / Alpha / Beta
//    - SukiSUULtra / suki-next
//
//  这些 fork 通常修改守护进程名、配置路径,但 root 级文件痕迹
//  仍然存在。本层从用户态文件 + 进程 cmdline 双线检测。
// ═══════════════════════════════════════════════════════════

#ifdef __cplusplus
extern "C" {
#endif

// 主检测入口:返回 true 表示检测到任意现代 root fork
bool detectModernRootForks(void);

// 单项检测 (供并行引擎调用)
bool detectSukiSU(void);
bool detectSukiSUUltra(void);
bool detectReZygisk(void);
bool detectReZygiskVariants(void);
bool detectMagiskDelta(void);
bool detectMagiskKitsune(void);
bool detectMagiskAlphaBeta(void);
bool detectZygiskNextDelta(void);

// 完整扫描报告,返回发现的项数
int modernRootForksFullScan(char* out_report, size_t out_size);

#ifdef __cplusplus
}
#endif

#endif // APEX_ROOT_LAYER17_MODERN_ROOT_FORKS_H
