#ifndef APEX_ROOT_LAYER15_DANGEROUS_APPS_H
#define APEX_ROOT_LAYER15_DANGEROUS_APPS_H

#include <cstddef>
// 第十五层 · 危险应用检测 (root 级)
// 检测 GameGuardian / CheatEngine / Lucky Patcher / GameKiller 等
// 这些应用通常需要 root 或本身就是 root 滥用工具

bool detectGameGuardian();
bool detectCheatEngine();
bool detectLuckyPatcher();
bool detectGameKiller();
bool detectMemoryEditors();
bool detectCrackingTools();
int  dangerousAppsFullScan(char* out_report, size_t out_size);

#endif // APEX_ROOT_LAYER15_DANGEROUS_APPS_H
