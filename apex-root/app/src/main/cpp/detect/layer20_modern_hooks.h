#ifndef APEX_ROOT_LAYER20_MODERN_HOOKS_H
#define APEX_ROOT_LAYER20_MODERN_HOOKS_H

#include <cstddef>

// ═══════════════════════════════════════════════════════════
//  第二十层 · 现代 Hook 框架检测 (root 级 + 用户态)
// ----------------------------------------------------------------
//  2024-2025 hook 生态演进:
//    - Pine / SandHook / FastHook (ART inline hook)
//    - Frida variants: frida-gum, frida-server, objection
//    - Modern Xposed: LSPatch (免 root LSPosed)
//    - Hookless: VirtualXposed variants
//    - Memory editors: GameGuardian variants
//    - Native injection: libinject.so variants
//
//  本层从 maps + 文件路径 + 进程 + 端口多线检测。
// ═══════════════════════════════════════════════════════════

#ifdef __cplusplus
extern "C" {
#endif

bool detectModernArtHooks(void);      // Pine/SandHook/FastHook/Epic
bool detectFridaVariants(void);       // frida-server/frida-gum/objection
bool detectLSPatch(void);             // 免 root LSPosed
bool detectModernMemoryEditors(void); // GG variants, ArtMoney, etc
bool detectNativeInjection(void);     // libinject, libsubstrate variants
bool detectModernHookFrameworks(void);

int modernHooksFullScan(char* out_report, size_t out_size);

#ifdef __cplusplus
}
#endif

#endif
