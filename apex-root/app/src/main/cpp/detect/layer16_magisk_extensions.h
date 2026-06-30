#ifndef APEX_ROOT_LAYER16_MAGISK_EXTENSIONS_H
#define APEX_ROOT_LAYER16_MAGISK_EXTENSIONS_H

#include <cstddef>
// 第十六层 · Magisk 扩展检测 (root 级)
// 检测 Magisk DenyList / Zygisk 模块 / LSPosed / Riru 模块 / etc.

bool detectMagiskDenyList();
bool detectZygiskModules();
bool detectLSPosedManager();
bool detectRiruModules();
bool detectModernForks();
int  magiskExtensionsFullScan(char* out_report, size_t out_size);

#endif // APEX_ROOT_LAYER16_MAGISK_EXTENSIONS_H
