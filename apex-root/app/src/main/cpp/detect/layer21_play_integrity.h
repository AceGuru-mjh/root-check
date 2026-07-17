#ifndef APEX_ROOT_LAYER21_PLAY_INTEGRITY_H
#define APEX_ROOT_LAYER21_PLAY_INTEGRITY_H

#include <cstddef>

// ═══════════════════════════════════════════════════════════
//  第二十一层 · Play Integrity API 检测 (root 级)
// ----------------------------------------------------------------
//  Google Play Integrity API 是 SafetyNet Attestation 的替代品,
//  2025 年起 SafetyNet 已废弃。Play Integrity 提供三个判定级别:
//    - MEETS_BASIC_INTEGRITY: 设备未被篡改 (基础)
//    - MEETS_DEVICE_INTEGRITY: 设备通过 Google 认证 (设备)
//    - MEETS_STRONG_INTEGRITY: 设备有可信硬件证明 (强)
//
//  Root 工具如 PlayIntegrityFix (PIF) 和 TrickyStore 会尝试
//  伪造 Play Integrity 判定。本层检测:
//    1. Google Play Services 是否安装 (无则无法检测)
//    2. 通过 Kotlin 层调用 Play Integrity API (需 JNI 回调)
//    3. 检测 PIF 模块残留 (/data/adb/modules/playintegrityfix)
// ═══════════════════════════════════════════════════════════

#ifdef __cplusplus
extern "C" {
#endif

// 主检测入口:返回 true 表示 Play Integrity 异常或检测到伪造
bool detectPlayIntegrityTampering(void);

// 单项检测
bool detectPIFModule(void);
bool detectTrickyStoreModule(void);
bool detectGmsAvailable(void);

// 完整扫描报告
int playIntegrityFullScan(char* out_report, size_t out_size);

#ifdef __cplusplus
}
#endif

#endif
