#include <jni.h>
#include <android/log.h>
#include <sys/system_properties.h>
#include <cstring>
#include <memory>
#include "bare_syscall/syscall_bridge.h"
#include "include/apex_common.h"
#include "detect/layer1_prop.h"
#include "detect/layer2_art.h"
#include "detect/layer3_mem.h"
#include "detect/layer4_mount.h"
#include "detect/layer5_sidechannel.h"
#include "detect/layer6_kernel.h"
#include "detect/layer7_boot.h"
#include "detect/layer8_magisk.h"
#include "detect/layer9_ksu.h"
#include "detect/layer10_apatch.h"
#include "detect/layer11_hook.h"
#include "detect/layer12_rom.h"
#include "detect/layer13_firmware.h"
#include "detect/layer14_virtualxposed.h"
#include "detect/layer15_dangerous_apps.h"
#include "detect/layer16_magisk_extensions.h"
#include "detect/layer17_modern_root_forks.h"
#include "detect/layer18_apatch_kpm.h"
#include "detect/layer19_hide_frameworks.h"
#include "detect/layer20_modern_hooks.h"
#include "detect/layer21_play_integrity.h"
#include "detect/layer22_emulator.h"
// P0-D10 注释 (v1.1.1): L23 预留未实现, detect/ 目录下编号从 layer22_emulator 直接
// 跳到 layer24_dhizuku。历史上 L23 曾计划用于"高级反调试 / 系统调用拦截检测",
// 因与 layer5_sidechannel 重叠未落地。如要补齐, 需重命名 layer24_dhizuku.* →
// layer23_dhizuku.* 并同步 CMakeLists.txt + JNI 函数命名 — 工作量较大, 暂不进行。
#include "detect/layer24_dhizuku.h"
#include "detect/selinux_context.h"
#include "detect/anti_hiding.h"
#include "namespace/namespace_isolation.h"
#include "ebpf/ebpf_manager.h"
#include "cure/cure_engine.h"
#include "guard/guard_engine.h"
#include "game/game_mode.h"
#include "hid/hwid_spoof.h"
#include "micro_services/engine/service_engine.h"

#define LOG_TAG "APEX-NATIVE"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#include "trusted_root/crypto/oqs_signature.h"
#include "trusted_root/crypto/crypto_primitives.h"

extern "C" {

// P1-4 修复: JNI_OnLoad 从 jni_bridge.cpp (未编译) 移到 native-lib.cpp (实际编译)
// 这样版本日志才能真正在库加载时打印
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    (void)vm;
    LOGI("Apex Agent v1.1.1 native library loaded (20-layer + parallel engine + UI rewrite)");
    return JNI_VERSION_1_6;
}

// ─────────────────────────────────────────────────────────────
// APEX-Detect: Full 20-layer detection (Ring3 root-level only)
// ----------------------------------------------------------------
// 已移除所有 Ring0 内核态检测：
//   - /proc/kallsyms 扫描
//   - /proc/modules 内核模块枚举
//   - /proc/sys/kernel/tainted 污染标志
//   - /sys/kpm sysfs KPM 节点
//   - /proc/kernelsu 内核 API
//   - syscall_table 符号检查
// 检测层 (v1.1.0 扩展到 20 层):
//   L1-L13: 原 16 层架构 (1-13)
//   L14: VirtualXposed / 太极 / 双开分身
//   L15: 危险应用 (GameGuardian / CE / Lucky Patcher 等)
//   L16: Magisk 扩展 (DenyList / ZygiskNext / ReZygisk / LSPosed / Riru)
//   L17: 现代 Root Fork (SukiSU / Magisk Delta / Kitsune / ReZygisk variants) [v1.1.0]
//   L18: APatch KPM 用户态 + Trampoline + KernelPatch [v1.1.0]
//   L19: 隐藏框架 (Zygisk-Assistant / AML / MagiskFrida / 持久化脚本) [v1.1.0]
//   L20: 现代 Hook 框架 (Pine/SandHook/ByteHook/ShadowHook/Frida variants/LSPatch) [v1.1.0]
// ─────────────────────────────────────────────────────────────

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_runQuickScanNative(JNIEnv* env, jobject) {
    std::string result = "=== APEX-Root Scan Result ===\n\n";

    bool l1 = detectSuspiciousProperties();
    bool l2 = detectArtInjection();
    bool l3 = detectSuspiciousMemory();
    bool l4 = detectSuspiciousMounts();
    bool l5 = detectSyscallTimingAnomaly();
    bool l6 = detectKernelTampering();          // 现为 root daemon 检测
    bool l7 = detectBootloaderStatus();
    bool l8 = detectMagiskDaemon();
    bool l9 = detectKernelSU();
    bool l10 = detectAPatch();
    bool l11 = detectXposedFramework();
    bool l12 = detectCustomROM();
    // 新增三层 root 级检测
    bool l14 = detectVirtualXposed();
    bool l15 = detectGameGuardian() || detectCheatEngine() || detectLuckyPatcher() ||
               detectGameKiller() || detectMemoryEditors() || detectCrackingTools();
    bool l16 = detectMagiskDenyList() || detectZygiskModules() ||
               detectLSPosedManager() || detectRiruModules() || detectModernForks();
    // v1.1.0 新增四层检测
    bool l17 = detectModernRootForks();
    bool l18 = detectAPatchKPMUsermode() || detectAPatchTrampoline() ||
               detectAPatchComponents() || detectKernelPatchProject();
    bool l19 = detectModernHideFrameworks();
    bool l20 = detectModernHookFrameworks();
    // v1.1.1 修复 P0-D9: L21-L24 接入主扫描流程 (此前只声明在 NativeBridge.kt 但未接入 runQuickScan)
    // L23 预留未实现 (详见 native-lib.cpp 顶部 include 注释)
    bool l21 = detectPlayIntegrityTampering() || detectPIFModule() || detectTrickyStoreModule();
    bool l22 = detectEmulator();
    bool l24 = detectDhizuku() || detectShizuku() || detectStellar();

    result += "L1 系统属性:     " + std::string(l1 ? "❌ 异常" : "✅ 正常") + "\n";
    result += "L2 ART注入:      " + std::string(l2 ? "❌ 异常" : "✅ 正常") + "\n";
    result += "L3 内存特征:     " + std::string(l3 ? "❌ 异常" : "✅ 正常") + "\n";
    result += "L4 挂载检查:     " + std::string(l4 ? "❌ 异常" : "✅ 正常") + "\n";
    result += "L5 侧信道:       " + std::string(l5 ? "❌ 异常" : "✅ 正常") + "\n";
    result += "L6 Root守护:     " + std::string(l6 ? "❌ 异常" : "✅ 正常") + "\n";
    result += "L7 Boot状态:     " + std::string(l7 ? "❌ 异常" : "✅ 正常") + "\n";
    result += "L8 Magisk:       " + std::string(l8 ? "❌ 异常" : "✅ 正常") + "\n";
    result += "L9 KernelSU:     " + std::string(l9 ? "❌ 异常" : "✅ 正常") + "\n";
    result += "L10 APatch:      " + std::string(l10 ? "❌ 异常" : "✅ 正常") + "\n";
    result += "L11 Hook框架:    " + std::string(l11 ? "❌ 异常" : "✅ 正常") + "\n";
    result += "L12 自定义ROM:   " + std::string(l12 ? "❌ 异常" : "✅ 正常") + "\n";
    result += "L14 虚拟框架:    " + std::string(l14 ? "❌ 异常" : "✅ 正常") + "\n";
    result += "L15 危险应用:    " + std::string(l15 ? "❌ 异常" : "✅ 正常") + "\n";
    result += "L16 Magisk扩展:  " + std::string(l16 ? "❌ 异常" : "✅ 正常") + "\n";
    result += "L17 Root Fork:   " + std::string(l17 ? "❌ 异常" : "✅ 正常") + "\n";
    result += "L18 APatch KPM:  " + std::string(l18 ? "❌ 异常" : "✅ 正常") + "\n";
    result += "L19 隐藏框架:    " + std::string(l19 ? "❌ 异常" : "✅ 正常") + "\n";
    result += "L20 现代Hook:    " + std::string(l20 ? "❌ 异常" : "✅ 正常") + "\n";
    result += "L21 PlayIntegrity:" + std::string(l21 ? "❌ 异常" : "✅ 正常") + "\n";
    result += "L22 模拟器:      " + std::string(l22 ? "❌ 异常" : "✅ 正常") + "\n";
    result += "L24 特权框架:    " + std::string(l24 ? "❌ 异常" : "✅ 正常") + "\n";

    int risk_count = (l1?1:0)+(l2?1:0)+(l3?1:0)+(l4?1:0)+(l5?1:0)+(l6?1:0)+(l7?1:0)+
                     (l8?1:0)+(l9?1:0)+(l10?1:0)+(l11?1:0)+(l12?1:0)+
                     (l14?1:0)+(l15?1:0)+(l16?1:0)+
                     (l17?1:0)+(l18?1:0)+(l19?1:0)+(l20?1:0)+
                     (l21?1:0)+(l22?1:0)+(l24?1:0);
    result += "\n风险指标: " + std::to_string(risk_count) + "/22\n";
    if (risk_count == 0) result += "结论: ✅ 设备安全\n";
    else if (risk_count <= 4) result += "结论: ⚠️ 轻度风险\n";
    else if (risk_count <= 9) result += "结论: ⚠️ 中等风险\n";
    else result += "结论: ❌ 高风险\n";

    return env->NewStringUTF(result.c_str());
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_isDeviceRootedNative(JNIEnv*, jobject) {
    return detectSuspiciousProperties() || detectArtInjection() ||
           detectSuspiciousMemory() || detectSuspiciousMounts() ||
           detectMagiskDaemon() || detectKernelSU() || detectAPatch() ||
           detectXposedFramework() || detectMagiskDenyList() ||
           detectZygiskModules() || detectLSPosedManager() ||
           detectRiruModules() || detectModernForks() ||
           // v1.1.0 新增检测层
           detectModernRootForks() ||
           detectAPatchKPMUsermode() || detectAPatchTrampoline() ||
           detectAPatchComponents() || detectKernelPatchProject() ||
           detectModernHideFrameworks() ||
           detectModernHookFrameworks() ||
           // v1.1.1 修复 P0-D9: L21-L24 接入 root 判定 (此前 isDeviceRooted 漏检这三层)
           detectPlayIntegrityTampering() || detectPIFModule() ||
           detectTrickyStoreModule() ||
           detectEmulator() ||
           detectDhizuku() || detectShizuku() || detectStellar();
}

// ─────────────────────────────────────────────────────────────
// APEX-Island: Namespace isolation
// ─────────────────────────────────────────────────────────────

JNIEXPORT jint JNICALL
Java_com_apex_root_island_NativeIsland_createIsolatedEnvNative(JNIEnv* env, jobject, jstring name) {
    if (!name) {
        LOGE("createIsolatedEnvNative: null name");
        return -1;
    }
    const char* cname = env->GetStringUTFChars(name, nullptr);
    if (!cname) {
        // OOM or JVM-broken — Java exception already pending
        return -1;
    }
    int pid = -1;
    // Ensure the JNI string is always released, even if the underlying
    // call throws. A thrown C++ exception would otherwise leak the UTF chars.
    try {
        pid = apex::island::create_isolated_environment(cname);
    } catch (const std::exception& e) {
        LOGE("create_isolated_environment failed: %s", e.what());
    } catch (...) {
        LOGE("create_isolated_environment failed: unknown exception");
    }
    env->ReleaseStringUTFChars(name, cname);
    return pid;
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_island_NativeIsland_destroyIsolatedEnvNative(JNIEnv*, jobject, jint pid) {
    return apex::island::destroy_isolated_environment(pid);
}

// ─────────────────────────────────────────────────────────────
// APEX-Cure: Repair system
// ─────────────────────────────────────────────────────────────

JNIEXPORT jint JNICALL
Java_com_apex_root_cure_NativeCure_detectRootTypeNative(JNIEnv*, jobject) {
    return static_cast<jint>(apex::cure::detect_root_solution());
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_cure_NativeCure_lightCleanupNative(JNIEnv*, jobject) {
    auto result = apex::cure::light_cleanup();
    return result.success;
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_cure_NativeCure_standardFixNative(JNIEnv*, jobject, jint root_type) {
    auto result = apex::cure::standard_fix(static_cast<apex::cure::RootType>(root_type));
    return result.success;
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_cure_NativeCure_deepRecoveryNative(JNIEnv*, jobject) {
    auto result = apex::cure::deep_recovery();
    return result.success;
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_cure_NativeCure_factoryResetNative(JNIEnv*, jobject) {
    auto result = apex::cure::factory_reset();
    return result.success;
}

// ─────────────────────────────────────────────────────────────
// APEX-Guard: Real-time protection
// ─────────────────────────────────────────────────────────────

JNIEXPORT jboolean JNICALL
Java_com_apex_root_guard_NativeGuard_startGuardianNative(JNIEnv*, jobject) {
    return apex::guard::start_guardian();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_guard_NativeGuard_checkIntegrityNative(JNIEnv*, jobject) {
    bool sys_ok = apex::guard::check_system_integrity();
    bool mod_ok = apex::guard::check_kernel_module_integrity();
    return sys_ok && mod_ok;
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_guard_NativeGuard_preventTamperNative(JNIEnv*, jobject) {
    return apex::guard::prevent_system_tamper();
}

// ─────────────────────────────────────────────────────────────
// Game Mode
// ─────────────────────────────────────────────────────────────

JNIEXPORT jboolean JNICALL
Java_com_apex_root_game_NativeGameMode_enterGameModeNative(JNIEnv*, jobject) {
    return apex::game::enter_game_mode();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_game_NativeGameMode_exitGameModeNative(JNIEnv*, jobject) {
    return apex::game::exit_game_mode();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_game_NativeGameMode_isInGameModeNative(JNIEnv*, jobject) {
    return apex::game::is_in_game_mode();
}

// ─────────────────────────────────────────────────────────────
// HWID Spoof
// ─────────────────────────────────────────────────────────────

JNIEXPORT jboolean JNICALL
Java_com_apex_root_hid_NativeHwid_spoofAllNative(JNIEnv*, jobject) {
    return apex::hid::spoof_all_hwids();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_hid_NativeHwid_restoreRealNative(JNIEnv*, jobject) {
    return apex::hid::restore_real_hwids();
}

// ─────────────────────────────────────────────────────────────
// Utility: quick root check
// ─────────────────────────────────────────────────────────────

JNIEXPORT jint JNICALL
Java_com_apex_root_data_jni_NativeBridge_getRiskScoreNative(JNIEnv*, jobject) {
    int score = 0;
    if (detectSuspiciousProperties()) score += 10;
    if (detectArtInjection()) score += 12;
    if (detectSuspiciousMemory()) score += 8;
    if (detectSuspiciousMounts()) score += 12;
    if (detectSyscallTimingAnomaly()) score += 8;
    if (detectKernelTampering()) score += 12;        // root daemon
    if (detectBootloaderStatus()) score += 8;
    if (detectMagiskDaemon()) score += 10;
    // 新增的扩展检测
    if (detectVirtualXposed()) score += 6;
    if (detectGameGuardian() || detectCheatEngine() ||
        detectLuckyPatcher() || detectGameKiller()) score += 6;
    if (detectMemoryEditors() || detectCrackingTools()) score += 5;
    if (detectMagiskDenyList()) score += 5;
    if (detectZygiskModules()) score += 6;
    if (detectLSPosedManager()) score += 5;
    if (detectRiruModules()) score += 4;
    if (detectModernForks()) score += 3;
    // v1.1.0 新增检测层评分
    if (detectModernRootForks()) score += 8;         // SukiSU/Magisk Delta 等现代 fork
    if (detectAPatchKPMUsermode() || detectAPatchTrampoline() ||
        detectAPatchComponents() || detectKernelPatchProject()) score += 8;
    if (detectModernHideFrameworks()) score += 7;    // Zygisk-Assistant/AML/MagiskFrida
    if (detectModernHookFrameworks()) score += 9;    // Pine/SandHook/Frida variants/LSPatch
    return score > 100 ? 100 : score;
}

// ─────────────────────────────────────────────────────────────
// Post-Quantum Crypto (liboqs Dilithium)
// ─────────────────────────────────────────────────────────────

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_isPostQuantumAvailableNative(JNIEnv*, jobject) {
    return apex::crypto::OqsSignature::getInstance().isAvailable();
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_getSignedReportNative(JNIEnv* env, jobject) {
    auto& oqs = apex::crypto::OqsSignature::getInstance();
    if (!oqs.isAvailable()) {
        return env->NewStringUTF("ERROR: Post-quantum crypto not available (liboqs not linked)");
    }

    // Run detection
    bool results[12] = {
        detectSuspiciousProperties(), detectArtInjection(),
        detectSuspiciousMemory(), detectSuspiciousMounts(),
        detectSyscallTimingAnomaly(), detectKernelTampering(),
        detectBootloaderStatus(), detectMagiskDaemon(),
        detectKernelSU(), detectAPatch(),
        detectXposedFramework(), detectCustomROM()
    };

    // Build report string
    std::string report = "=== APEX-Root Signed Detection Report ===\n";
    // P1-3 修复: 使用 apex::VERSION_STRING 而非硬编码
    report += "Version: ";
    report += apex::VERSION_STRING;
    report += "\n";
    report += "Timestamp: " + std::to_string(bs_clock_ns()) + "\n\n";

    for (int i = 0; i < 12; i++) {
        report += "L" + std::to_string(i + 1) + ": " + (results[i] ? "DETECTED" : "CLEAN") + "\n";
    }

    int risk = 0;
    for (auto r : results) if (r) risk++;
    report += "\nRisk Score: " + std::to_string(risk) + "/12\n";

    // Sign the report with a fresh keypair
    std::vector<uint8_t> pub, priv, report_bytes = oqs.stringToBytes(report);
    if (oqs.generateKeyPair(pub, priv)) {
        auto signature = oqs.sign(report_bytes, priv);
        report += "\n--- Post-Quantum Signature ---\n";
        report += "Algorithm: ML-DSA-65 (CRYSTALS-Dilithium-3)\n";
        report += "Public Key: " + oqs.base64Encode(pub) + "\n";
        report += "Signature: " + oqs.base64Encode(signature) + "\n";
        report += "Signature Verified: " +
            std::string(oqs.verify(report_bytes, signature, pub) ? "PASS" : "FAIL") + "\n";
    }

    return env->NewStringUTF(report.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_getPostQuantumInfoNative(JNIEnv* env, jobject) {
    auto& oqs = apex::crypto::OqsSignature::getInstance();
    std::string info;
    info += "liboqs Available: " + std::string(oqs.isAvailable() ? "YES" : "NO") + "\n";
    info += "Algorithm: ML-DSA-65 (CRYSTALS-Dilithium-3)\n";
    info += "Public Key Size: " + std::to_string(oqs.publicKeySize()) + " bytes\n";
    info += "Secret Key Size: " + std::to_string(oqs.secretKeySize()) + " bytes\n";
    info += "Signature Size: " + std::to_string(oqs.signatureSize()) + " bytes\n";
    return env->NewStringUTF(info.c_str());
}

// ─────────────────────────────────────────────────────────────
// 新增：L14 / L15 / L16 完整扫描接口
// ─────────────────────────────────────────────────────────────

// Helper: safely build a Java String from a variable-size native scan report.
// Scanning 50+ framework paths can easily overflow the old 4KB stack buffer,
// corrupting the return address and crashing the app with SIGSEGV. Heap-
// allocate a larger buffer and guarantee NUL-termination.
static jstring report_to_jstring(JNIEnv* env, int (*scanner)(char*, size_t), size_t initial_size = 16384) {
    if (!env || !scanner) return env ? env->NewStringUTF("") : nullptr;

    size_t cap = initial_size;
    // Cap at 256KB to bound memory in pathological cases.
    while (cap <= 256 * 1024) {
        auto buf = std::make_unique<char[]>(cap);
        if (!buf) return env->NewStringUTF("");
        buf[0] = '\0';
        scanner(buf.get(), cap);
        // The scanners always NUL-terminate within out_size, so strlen is safe.
        size_t used = strlen(buf.get());
        // If we used >= cap-1 the output was truncated — retry with a bigger buffer.
        if (used < cap - 1) {
            return env->NewStringUTF(buf.get());
        }
        cap *= 2;
    }
    // Gave up at the cap — return whatever the last scan produced.
    auto buf = std::make_unique<char[]>(cap);
    if (!buf) return env->NewStringUTF("");
    buf[0] = '\0';
    scanner(buf.get(), cap);
    return env->NewStringUTF(buf.get());
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_virtualXposedFullScanNative(JNIEnv* env, jobject) {
    return report_to_jstring(env, [](char* b, size_t s) -> int {
        return virtualXposedFullScan(b, s);
    });
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_dangerousAppsFullScanNative(JNIEnv* env, jobject) {
    return report_to_jstring(env, [](char* b, size_t s) -> int {
        return dangerousAppsFullScan(b, s);
    });
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_magiskExtensionsFullScanNative(JNIEnv* env, jobject) {
    return report_to_jstring(env, [](char* b, size_t s) -> int {
        return magiskExtensionsFullScan(b, s);
    });
}

// ─────────────────────────────────────────────────────────────
// v1.1.0 新增: L17-L20 完整扫描接口
// ─────────────────────────────────────────────────────────────

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_modernRootForksFullScanNative(JNIEnv* env, jobject) {
    return report_to_jstring(env, [](char* b, size_t s) -> int {
        return modernRootForksFullScan(b, s);
    });
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_apatchKpmFullScanNative(JNIEnv* env, jobject) {
    return report_to_jstring(env, [](char* b, size_t s) -> int {
        return apatchKpmFullScan(b, s);
    });
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_hideFrameworksFullScanNative(JNIEnv* env, jobject) {
    return report_to_jstring(env, [](char* b, size_t s) -> int {
        return hideFrameworksFullScan(b, s);
    });
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_modernHooksFullScanNative(JNIEnv* env, jobject) {
    return report_to_jstring(env, [](char* b, size_t s) -> int {
        return modernHooksFullScan(b, s);
    });
}

// ─── L17-L20 单项 boolean 检测 ───

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectModernRootForksV2Native(JNIEnv*, jobject) {
    return detectModernRootForks();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectSukiSUNative(JNIEnv*, jobject) {
    return detectSukiSU();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectSukiSUUltraNative(JNIEnv*, jobject) {
    return detectSukiSUUltra();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectReZygiskV2Native(JNIEnv*, jobject) {
    return detectReZygisk();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectMagiskDeltaNative(JNIEnv*, jobject) {
    return detectMagiskDelta();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectAPatchKPMNative(JNIEnv*, jobject) {
    return detectAPatchKPMUsermode();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectAPatchTrampolineNative(JNIEnv*, jobject) {
    return detectAPatchTrampoline();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectKernelPatchProjectNative(JNIEnv*, jobject) {
    return detectKernelPatchProject();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectZygiskAssistantNative(JNIEnv*, jobject) {
    return detectZygiskAssistant();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectPersistentScriptsNative(JNIEnv*, jobject) {
    return detectPersistentScripts();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectModernArtHooksNative(JNIEnv*, jobject) {
    return detectModernArtHooks();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectFridaVariantsNative(JNIEnv*, jobject) {
    return detectFridaVariants();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectLSPatchNative(JNIEnv*, jobject) {
    return detectLSPatch();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectModernHookFrameworksNative(JNIEnv*, jobject) {
    return detectModernHookFrameworks();
}

// ─── v1.0.5 新增 L21-L24 ───

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectPlayIntegrityTamperingNative(JNIEnv*, jobject) {
    return detectPlayIntegrityTampering();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectPIFModuleNative(JNIEnv*, jobject) {
    return detectPIFModule();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectTrickyStoreModuleNative(JNIEnv*, jobject) {
    return detectTrickyStoreModule();
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_playIntegrityFullScanNative(JNIEnv* env, jobject) {
    return report_to_jstring(env, [](char* b, size_t s) -> int {
        return playIntegrityFullScan(b, s);
    });
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectEmulatorNative(JNIEnv*, jobject) {
    return detectEmulator();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectQEMUNative(JNIEnv*, jobject) {
    return detectQEMU();
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_emulatorFullScanNative(JNIEnv* env, jobject) {
    return report_to_jstring(env, [](char* b, size_t s) -> int {
        return emulatorFullScan(b, s);
    });
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectDhizukuNative(JNIEnv*, jobject) {
    return detectDhizuku();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectShizukuNative(JNIEnv*, jobject) {
    return detectShizuku();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectStellarNative(JNIEnv*, jobject) {
    return detectStellar();
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_dhizukuFullScanNative(JNIEnv* env, jobject) {
    return report_to_jstring(env, [](char* b, size_t s) -> int {
        return dhizukuFullScan(b, s);
    });
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectVirtualXposedNative(JNIEnv*, jobject) {
    return detectVirtualXposed();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectTaiChiNative(JNIEnv*, jobject) {
    return detectTaiChi();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectDualSpaceAppsNative(JNIEnv*, jobject) {
    return detectDualSpaceApps();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectGameGuardianNative(JNIEnv*, jobject) {
    return detectGameGuardian();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectCheatEngineNative(JNIEnv*, jobject) {
    return detectCheatEngine();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectLuckyPatcherNative(JNIEnv*, jobject) {
    return detectLuckyPatcher();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectMemoryEditorsNative(JNIEnv*, jobject) {
    return detectMemoryEditors();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectCrackingToolsNative(JNIEnv*, jobject) {
    return detectCrackingTools();
}

// ─── Frida 检测（layer11_hook.cpp::detectFrida）──────────
JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectFridaNative(JNIEnv*, jobject) {
    return detectFrida();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectMagiskDenyListNative(JNIEnv*, jobject) {
    return detectMagiskDenyList();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectZygiskModulesNative(JNIEnv*, jobject) {
    return detectZygiskModules();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectLSPosedManagerNative(JNIEnv*, jobject) {
    return detectLSPosedManager();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectRiruModulesNative(JNIEnv*, jobject) {
    return detectRiruModules();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectModernForksNative(JNIEnv*, jobject) {
    return detectModernForks();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectHideMyApplistNative(JNIEnv*, jobject) {
    return detectHideMyApplist();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectStorageIsolationNative(JNIEnv*, jobject) {
    return detectStorageIsolation();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectMagiskHideLegacyNative(JNIEnv*, jobject) {
    return detectMagiskHideLegacy();
}

// P2-7: 删除孤儿 JNI 导出 detectMagiskDenyListCfgNative (Kotlin 未声明,功能重复)
// 原代码: JNIEXPORT jboolean Java_..._detectMagiskDenyListCfgNative { return detectMagiskDenyList(); }

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectSyscallResultInconsistencyNative(JNIEnv*, jobject) {
    return detectSyscallResultInconsistency();
}

// ─────────────────────────────────────────────────────────────
// Enhanced detection: Memory fingerprint deep scan
// ─────────────────────────────────────────────────────────────

JNIEXPORT jint JNICALL
Java_com_apex_root_data_jni_NativeBridge_fullMemoryFingerprintNative(JNIEnv*, jobject) {
    return fullMemoryFingerprintScan();
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_deepMemoryScanReportNative(JNIEnv* env, jobject) {
    return report_to_jstring(env, [](char* b, size_t s) -> int {
        return deepMemoryFingerprintScan(b, s);
    });
}

JNIEXPORT jint JNICALL
Java_com_apex_root_data_jni_NativeBridge_countRWXPagesNative(JNIEnv*, jobject) {
    return countRWXPages();
}

// ─────────────────────────────────────────────────────────────
// Enhanced detection: SELinux context jump
// ─────────────────────────────────────────────────────────────

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectSELinuxContextJumpNative(JNIEnv*, jobject) {
    return detectSELinuxContextJump();
}

// ─────────────────────────────────────────────────────────────
// eBPF capability probe (for Hide Mode UX)
// ─────────────────────────────────────────────────────────────

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_getEbpfCapabilityReportNative(JNIEnv* env, jobject) {
    apex::ebpf::EbpfCapability cap;
    apex::ebpf::detect_ebpf_capabilities(&cap);
    bool use_ebpf = apex::ebpf::should_use_ebpf_path();

    char buf[1024];
    int n = snprintf(buf, sizeof(buf),
        "=== eBPF Capability Report ===\n"
        "Kernel: %d.%d (%s)\n"
        "SELinux mode: %s\n"
        "BPF syscall present: %s\n"
        "bpffs mounted (/sys/fs/bpf): %s\n"
        "Kernel version >= 5.10: %s\n"
        "SELinux likely allows bpf: %s\n"
        "BPF_MAP_CREATE success: %s\n"
        "BPF_PROG_LOAD success: %s\n"
        "→ should_use_ebpf_path: %s\n",
        cap.kernel_major, cap.kernel_minor,
        cap.kernel_version_ok ? "OK" : "too old",
        cap.selinux_mode,
        cap.bpf_syscall_present ? "yes" : "no",
        cap.bpffs_mounted ? "yes" : "no",
        cap.kernel_version_ok ? "yes" : "no",
        cap.selinux_allows_bpf ? "yes" : "no",
        cap.can_create_map ? "yes" : "no",
        cap.can_load_prog ? "yes" : "no",
        use_ebpf ? "YES (eBPF hide-mode active)" : "NO (fallback to mount-ns + rename)");
    if (n < 0) return env->NewStringUTF("ERROR: snprintf failed");
    return env->NewStringUTF(buf);
}

// ─────────────────────────────────────────────────────────────
// P1-1 修复: 设置微服务插件目录 (从 Kotlin 传入 nativeLibraryDir + "/plugins")
// 必须在 service_engine::initialize() 之前调用
// ─────────────────────────────────────────────────────────────

JNIEXPORT void JNICALL
Java_com_apex_root_data_jni_NativeBridge_setPluginsDirNative(JNIEnv* env, jobject, jstring path) {
    if (!path) {
        LOGE("setPluginsDirNative: null path");
        return;
    }
    const char* cpath = env->GetStringUTFChars(path, nullptr);
    if (!cpath) {
        LOGE("setPluginsDirNative: GetStringUTFChars returned null");
        return;
    }
    apex::engine::service_engine::set_plugins_dir(cpath);
    env->ReleaseStringUTFChars(path, cpath);
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectSELinuxPolicyModNative(JNIEnv*, jobject) {
    return detectSELinuxPolicyModification();
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_selinuxFullScanNative(JNIEnv* env, jobject) {
    return report_to_jstring(env, [](char* b, size_t s) -> int {
        return runSELinuxFullScan(b, s);
    }, 4096);
}

// ─────────────────────────────────────────────────────────────
// Enhanced detection: Anti-hiding probes (Shamiko / ZygiskNext)
// ─────────────────────────────────────────────────────────────

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectShamikoNative(JNIEnv*, jobject) {
    return detectShamiko();
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_shamikoFullScanNative(JNIEnv* env, jobject) {
    return report_to_jstring(env, [](char* b, size_t s) -> int {
        return shamikoFullScan(b, s);
    }, 4096);
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectZygiskNextNative(JNIEnv*, jobject) {
    return detectZygiskNext();
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_zygiskNextFullScanNative(JNIEnv* env, jobject) {
    return report_to_jstring(env, [](char* b, size_t s) -> int {
        return zygiskNextFullScan(b, s);
    }, 4096);
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectProcessHidingNative(JNIEnv*, jobject) {
    return detectProcessHiding();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectMountNamespaceHidingNative(JNIEnv*, jobject) {
    return detectMountNamespaceHiding();
}

// 已移除：detectSyscallTableHook —— 原 Ring0 检测（依赖 /proc/kallsyms）
// 改用 detectSyscallResultInconsistency 替代（用户态 syscall 结果一致性检测）

// ─────────────────────────────────────────────────────────────
// ART enhanced detection
// ─────────────────────────────────────────────────────────────

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_artEnhancedScanNative(JNIEnv* env, jobject) {
    return report_to_jstring(env, [](char* b, size_t s) -> int {
        return detectArtEnhanced(b, s);
    }, 4096);
}

// ─── Xposed 框架检测（layer11_hook.cpp::detectXposedFramework）──
JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectXposedFrameworkNative(JNIEnv*, jobject) {
    return detectXposedFramework();
}

// ─────────────────────────────────────────────────────────────
// Firmware partition integrity
// ─────────────────────────────────────────────────────────────

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectFirmwareTamperingNative(JNIEnv*, jobject) {
    return detectFirmwareTampering();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectAVBStatusNative(JNIEnv*, jobject) {
    return detectAVBStatus();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectCustomRecoveryNative(JNIEnv*, jobject) {
    return detectRecoveryPartition();
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_firmwareFullScanNative(JNIEnv* env, jobject) {
    return report_to_jstring(env, [](char* b, size_t s) -> int {
        return firmwareFullScan(b, s);
    }, 4096);
}

// ─────────────────────────────────────────────────────────────
// Device identity / crypto helpers
// ─────────────────────────────────────────────────────────────

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_getDeviceIdentifierNative(JNIEnv* env, jobject) {
    // 512 bytes comfortably holds: serial (≤ 64) + hostname (≤ 256) + 20-digit ns + separators
    char buf[512];
    // Build device ID from system properties
    auto read_prop = [](const char* prop) -> std::string {
        FILE* f = fopen(prop, "r");
        if (!f) return "";
        // Hostnames on Linux can be up to 256 bytes; sized accordingly.
        char line[256] = {};
        if (fgets(line, sizeof(line), f)) {
            fclose(f);
            std::string s(line);
            while (!s.empty() && (s.back() == '\n' || s.back() == ' ')) s.pop_back();
            return s;
        }
        fclose(f);
        return "";
    };

    // Read Android serial number via system property API
    auto read_system_property = [](const char* key) -> std::string {
        // PROP_VALUE_MAX is 92 on Android, but be defensive.
        char val[128] = {};
        int ret = __system_property_get(key, val);
        if (ret > 0) return std::string(val);
        return "";
    };

    auto serial = read_system_property("ro.serialno");
    auto brand = read_prop("/proc/sys/kernel/hostname");

    int n = snprintf(buf, sizeof(buf), "%s-%s-%lld",
        serial.empty() ? "unknown" : serial.c_str(),
        brand.empty() ? "android" : brand.c_str(),
        (long long)bs_clock_ns());
    if (n < 0) return env->NewStringUTF("apex-root-default");
    // snprintf always NUL-terminates within sizeof(buf), so no overflow risk.
    // If truncation happened (n >= sizeof(buf)), surface it rather than return
    // a misleadingly short identifier.
    if ((size_t)n >= sizeof(buf)) {
        LOGE("Device identifier truncated (need %d bytes)", n);
        return env->NewStringUTF("apex-root-truncated");
    }
    return env->NewStringUTF(buf);
}

JNIEXPORT jbyteArray JNICALL
Java_com_apex_root_data_jni_NativeBridge_sha3_512Native(JNIEnv* env, jobject, jbyteArray data) {
    if (!data) {
        LOGE("sha3_512Native: null data array");
        return env->NewByteArray(0);
    }
    jsize len = env->GetArrayLength(data);
    if (len < 0) {
        LOGE("sha3_512Native: negative array length");
        return env->NewByteArray(0);
    }
    jbyte* bytes = env->GetByteArrayElements(data, nullptr);
    if (!bytes) {
        // JVM OOM or pending exception
        return env->NewByteArray(0);
    }
    auto hash = apex::crypto::sha3_512(
        reinterpret_cast<const uint8_t*>(bytes), static_cast<size_t>(len));
    env->ReleaseByteArrayElements(data, bytes, JNI_ABORT);

    jbyteArray result = env->NewByteArray(64);
    if (result) {
        env->SetByteArrayRegion(result, 0, 64, reinterpret_cast<const jbyte*>(hash.data()));
    }
    return result;
}

} // extern "C"
