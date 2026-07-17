#ifndef APEX_ROOT_LAYER22_EMULATOR_H
#define APEX_ROOT_LAYER22_EMULATOR_H

#include <cstddef>

// ═══════════════════════════════════════════════════════════
//  第二十二层 · 模拟器检测 (root 级 + 用户态)
// ----------------------------------------------------------------
//  检测常见 Android 模拟器:QEMU/genymotion/bluestacks/
//  Android SDK emulator/Nox/LDPlayer/MEmu。
//  从 CPU 特征、硬件指纹、传感器、IP 地址等多维度检测。
// ═══════════════════════════════════════════════════════════

#ifdef __cplusplus
extern "C" {
#endif

bool detectEmulator(void);
bool detectQEMU(void);
bool detectGenymotion(void);
bool detectBluestacks(void);
bool detectNoxOrLDPlayer(void);
bool detectGenericEmulatorProps(void);

int emulatorFullScan(char* out_report, size_t out_size);

#ifdef __cplusplus
}
#endif

#endif
