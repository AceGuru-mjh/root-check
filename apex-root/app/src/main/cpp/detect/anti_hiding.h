#ifndef APEX_ROOT_ANTI_HIDING_H
#define APEX_ROOT_ANTI_HIDING_H

#include <cstddef>

// Shamiko detection
bool detectShamiko();
bool detectShamikoSELinuxTrick();
bool detectShamikoWhiteList();
int  shamikoFullScan(char* out_report, size_t out_size);

// ZygiskNext detection
bool detectZygiskNext();
bool detectZygiskNextMemfd();
bool detectZygiskNextIsolation();
int  zygiskNextFullScan(char* out_report, size_t out_size);

// Generic anti-hiding probes (root 级)
bool detectProcessHiding();
bool detectMountNamespaceHiding();

// 新增：额外 Root 隐藏框架检测 (root 级)
bool detectHideMyApplist();
bool detectStorageIsolation();
bool detectMagiskHideLegacy();
bool detectMagiskDenyList();

// 已移除：detectSyscallTableHook() — 原 Ring0 检测，依赖 /proc/kallsyms
// 如需检测 syscall hook，请使用 layer5_sidechannel.cpp 的时延分析

#endif // APEX_ROOT_ANTI_HIDING_H
