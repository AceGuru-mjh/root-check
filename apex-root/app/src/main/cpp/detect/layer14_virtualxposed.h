#ifndef APEX_ROOT_LAYER14_VIRTUALXPOSED_H
#define APEX_ROOT_LAYER14_VIRTUALXPOSED_H

#include <cstddef>

// 第十四层 · 虚拟框架 / 双开分身检测 (root 级)
// 检测 VirtualXposed / 太极·阳 / 太极·阴 / 分身空间 / 双开应用等
// 这些框架不需要 root 即可注入 Xposed 模块到目标进程

bool detectVirtualXposed();
bool detectTaiChi();
bool detectDualSpaceApps();
bool detectAppCloningFrameworks();
int  virtualXposedFullScan(char* out_report, size_t out_size);

#endif // APEX_ROOT_LAYER14_VIRTUALXPOSED_H
