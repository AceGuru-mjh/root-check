#include <jni.h>
#include <android/log.h>
#include <sys/system_properties.h>
#include <cstring>
#include "bare_syscall/syscall_bridge.h"
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
#include "detect/selinux_context.h"
#include "detect/anti_hiding.h"
#include "namespace/namespace_isolation.h"
#include "ebpf/ebpf_manager.h"
#include "cure/cure_engine.h"
#include "guard/guard_engine.h"
#include "game/game_mode.h"
#include "hid/hwid_spoof.h"

#define LOG_TAG "APEX-NATIVE"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#include "trusted_root/crypto/oqs_signature.h"
#include "trusted_root/crypto/crypto_primitives.h"

extern "C" {

// ─────────────────────────────────────────────────────────────
// APEX-Detect: Full 16-layer detection (Ring3 root-level only)
// ----------------------------------------------------------------
// 已移除所有 Ring0 内核态检测：
//   - /proc/kallsyms 扫描
//   - /proc/modules 内核模块枚举
//   - /proc/sys/kernel/tainted 污染标志
//   - /sys/kpm sysfs KPM 节点
//   - /proc/kernelsu 内核 API
//   - syscall_table 符号检查
// 新增检测层：
//   - L14: VirtualXposed / 太极 / 双开分身
//   - L15: 危险应用 (GameGuardian / CE / Lucky Patcher 等)
//   - L16: Magisk 扩展 (DenyList / ZygiskNext / ReZygisk / LSPosed / Riru)
// ─────────────────────────────────────────────────────────────

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_runQuickScan(JNIEnv* env, jobject) {
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

    int risk_count = (l1?1:0)+(l2?1:0)+(l3?1:0)+(l4?1:0)+(l5?1:0)+(l6?1:0)+(l7?1:0)+
                     (l8?1:0)+(l9?1:0)+(l10?1:0)+(l11?1:0)+(l12?1:0)+
                     (l14?1:0)+(l15?1:0)+(l16?1:0);
    result += "\n风险指标: " + std::to_string(risk_count) + "/15\n";
    if (risk_count == 0) result += "结论: ✅ 设备安全\n";
    else if (risk_count <= 3) result += "结论: ⚠️ 轻度风险\n";
    else if (risk_count <= 7) result += "结论: ⚠️ 中等风险\n";
    else result += "结论: ❌ 高风险\n";

    return env->NewStringUTF(result.c_str());
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_isDeviceRooted(JNIEnv*, jobject) {
    return detectSuspiciousProperties() || detectArtInjection() ||
           detectSuspiciousMemory() || detectSuspiciousMounts() ||
           detectMagiskDaemon() || detectKernelSU() || detectAPatch() ||
           detectXposedFramework() || detectMagiskDenyList() ||
           detectZygiskModules() || detectLSPosedManager() ||
           detectRiruModules() || detectModernForks();
}

// ─────────────────────────────────────────────────────────────
// APEX-Island: Namespace isolation
// ─────────────────────────────────────────────────────────────

JNIEXPORT jint JNICALL
Java_com_apex_root_island_NativeIsland_createIsolatedEnv(JNIEnv* env, jobject, jstring name) {
    const char* cname = env->GetStringUTFChars(name, nullptr);
    int pid = apex::island::create_isolated_environment(cname);
    env->ReleaseStringUTFChars(name, cname);
    return pid;
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_island_NativeIsland_destroyIsolatedEnv(JNIEnv*, jobject, jint pid) {
    return apex::island::destroy_isolated_environment(pid);
}

// ─────────────────────────────────────────────────────────────
// APEX-Cure: Repair system
// ─────────────────────────────────────────────────────────────

JNIEXPORT jint JNICALL
Java_com_apex_root_cure_NativeCure_detectRootType(JNIEnv*, jobject) {
    return static_cast<jint>(apex::cure::detect_root_solution());
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_cure_NativeCure_lightCleanup(JNIEnv*, jobject) {
    auto result = apex::cure::light_cleanup();
    return result.success;
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_cure_NativeCure_standardFix(JNIEnv*, jobject, jint root_type) {
    auto result = apex::cure::standard_fix(static_cast<apex::cure::RootType>(root_type));
    return result.success;
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_cure_NativeCure_deepRecovery(JNIEnv*, jobject) {
    auto result = apex::cure::deep_recovery();
    return result.success;
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_cure_NativeCure_factoryReset(JNIEnv*, jobject) {
    auto result = apex::cure::factory_reset();
    return result.success;
}

// ─────────────────────────────────────────────────────────────
// APEX-Guard: Real-time protection
// ─────────────────────────────────────────────────────────────

JNIEXPORT jboolean JNICALL
Java_com_apex_root_guard_NativeGuard_startGuardian(JNIEnv*, jobject) {
    return apex::guard::start_guardian();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_guard_NativeGuard_checkIntegrity(JNIEnv*, jobject) {
    bool sys_ok = apex::guard::check_system_integrity();
    bool mod_ok = apex::guard::check_kernel_module_integrity();
    return sys_ok && mod_ok;
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_guard_NativeGuard_preventTamper(JNIEnv*, jobject) {
    return apex::guard::prevent_system_tamper();
}

// ─────────────────────────────────────────────────────────────
// Game Mode
// ─────────────────────────────────────────────────────────────

JNIEXPORT jboolean JNICALL
Java_com_apex_root_game_NativeGameMode_enterGameMode(JNIEnv*, jobject) {
    return apex::game::enter_game_mode();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_game_NativeGameMode_exitGameMode(JNIEnv*, jobject) {
    return apex::game::exit_game_mode();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_game_NativeGameMode_isInGameMode(JNIEnv*, jobject) {
    return apex::game::is_in_game_mode();
}

// ─────────────────────────────────────────────────────────────
// HWID Spoof
// ─────────────────────────────────────────────────────────────

JNIEXPORT jboolean JNICALL
Java_com_apex_root_hid_NativeHwid_spoofAll(JNIEnv*, jobject) {
    return apex::hid::spoof_all_hwids();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_hid_NativeHwid_restoreReal(JNIEnv*, jobject) {
    return apex::hid::restore_real_hwids();
}

// ─────────────────────────────────────────────────────────────
// Utility: quick root check
// ─────────────────────────────────────────────────────────────

JNIEXPORT jint JNICALL
Java_com_apex_root_data_jni_NativeBridge_getRiskScore(JNIEnv*, jobject) {
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
    return score > 100 ? 100 : score;
}

// ─────────────────────────────────────────────────────────────
// Post-Quantum Crypto (liboqs Dilithium)
// ─────────────────────────────────────────────────────────────

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_isPostQuantumAvailable(JNIEnv*, jobject) {
    return apex::crypto::OqsSignature::getInstance().isAvailable();
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_getSignedReport(JNIEnv* env, jobject) {
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
    report += "Version: 1.0.2\n";
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
Java_com_apex_root_data_jni_NativeBridge_getPostQuantumInfo(JNIEnv* env, jobject) {
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

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_virtualXposedFullScan(JNIEnv* env, jobject) {
    char report[4096];
    report[0] = '\0';
    virtualXposedFullScan(report, sizeof(report));
    return env->NewStringUTF(report);
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_dangerousAppsFullScan(JNIEnv* env, jobject) {
    char report[4096];
    report[0] = '\0';
    dangerousAppsFullScan(report, sizeof(report));
    return env->NewStringUTF(report);
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_magiskExtensionsFullScan(JNIEnv* env, jobject) {
    char report[4096];
    report[0] = '\0';
    magiskExtensionsFullScan(report, sizeof(report));
    return env->NewStringUTF(report);
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectVirtualXposed(JNIEnv*, jobject) {
    return detectVirtualXposed();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectTaiChi(JNIEnv*, jobject) {
    return detectTaiChi();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectDualSpaceApps(JNIEnv*, jobject) {
    return detectDualSpaceApps();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectGameGuardian(JNIEnv*, jobject) {
    return detectGameGuardian();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectCheatEngine(JNIEnv*, jobject) {
    return detectCheatEngine();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectLuckyPatcher(JNIEnv*, jobject) {
    return detectLuckyPatcher();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectMemoryEditors(JNIEnv*, jobject) {
    return detectMemoryEditors();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectCrackingTools(JNIEnv*, jobject) {
    return detectCrackingTools();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectMagiskDenyList(JNIEnv*, jobject) {
    return detectMagiskDenyList();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectZygiskModules(JNIEnv*, jobject) {
    return detectZygiskModules();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectLSPosedManager(JNIEnv*, jobject) {
    return detectLSPosedManager();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectRiruModules(JNIEnv*, jobject) {
    return detectRiruModules();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectModernForks(JNIEnv*, jobject) {
    return detectModernForks();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectHideMyApplist(JNIEnv*, jobject) {
    return detectHideMyApplist();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectStorageIsolation(JNIEnv*, jobject) {
    return detectStorageIsolation();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectMagiskHideLegacy(JNIEnv*, jobject) {
    return detectMagiskHideLegacy();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectMagiskDenyListCfg(JNIEnv*, jobject) {
    return detectMagiskDenyList();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectSyscallResultInconsistency(JNIEnv*, jobject) {
    return detectSyscallResultInconsistency();
}

// ─────────────────────────────────────────────────────────────
// Enhanced detection: Memory fingerprint deep scan
// ─────────────────────────────────────────────────────────────

JNIEXPORT jint JNICALL
Java_com_apex_root_data_jni_NativeBridge_fullMemoryFingerprint(JNIEnv*, jobject) {
    return fullMemoryFingerprintScan();
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_deepMemoryScanReport(JNIEnv* env, jobject) {
    char report[4096];
    report[0] = '\0';
    deepMemoryFingerprintScan(report, sizeof(report));
    return env->NewStringUTF(report);
}

JNIEXPORT jint JNICALL
Java_com_apex_root_data_jni_NativeBridge_countRWXPages(JNIEnv*, jobject) {
    return countRWXPages();
}

// ─────────────────────────────────────────────────────────────
// Enhanced detection: SELinux context jump
// ─────────────────────────────────────────────────────────────

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectSELinuxContextJump(JNIEnv*, jobject) {
    return detectSELinuxContextJump();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectSELinuxPolicyMod(JNIEnv*, jobject) {
    return detectSELinuxPolicyModification();
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_selinuxFullScan(JNIEnv* env, jobject) {
    char report[2048];
    report[0] = '\0';
    runSELinuxFullScan(report, sizeof(report));
    return env->NewStringUTF(report);
}

// ─────────────────────────────────────────────────────────────
// Enhanced detection: Anti-hiding probes (Shamiko / ZygiskNext)
// ─────────────────────────────────────────────────────────────

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectShamiko(JNIEnv*, jobject) {
    return detectShamiko();
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_shamikoFullScan(JNIEnv* env, jobject) {
    char report[2048];
    report[0] = '\0';
    shamikoFullScan(report, sizeof(report));
    return env->NewStringUTF(report);
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectZygiskNext(JNIEnv*, jobject) {
    return detectZygiskNext();
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_zygiskNextFullScan(JNIEnv* env, jobject) {
    char report[2048];
    report[0] = '\0';
    zygiskNextFullScan(report, sizeof(report));
    return env->NewStringUTF(report);
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectProcessHiding(JNIEnv*, jobject) {
    return detectProcessHiding();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectMountNamespaceHiding(JNIEnv*, jobject) {
    return detectMountNamespaceHiding();
}

// 已移除：detectSyscallTableHook —— 原 Ring0 检测（依赖 /proc/kallsyms）
// 改用 detectSyscallResultInconsistency 替代（用户态 syscall 结果一致性检测）

// ─────────────────────────────────────────────────────────────
// ART enhanced detection
// ─────────────────────────────────────────────────────────────

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_artEnhancedScan(JNIEnv* env, jobject) {
    char report[2048];
    report[0] = '\0';
    detectArtEnhanced(report, sizeof(report));
    return env->NewStringUTF(report);
}

// ─────────────────────────────────────────────────────────────
// Firmware partition integrity
// ─────────────────────────────────────────────────────────────

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectFirmwareTampering(JNIEnv*, jobject) {
    return detectFirmwareTampering();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectAVBStatus(JNIEnv*, jobject) {
    return detectAVBStatus();
}

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectCustomRecovery(JNIEnv*, jobject) {
    return detectRecoveryPartition();
}

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_firmwareFullScan(JNIEnv* env, jobject) {
    char report[2048];
    report[0] = '\0';
    firmwareFullScan(report, sizeof(report));
    return env->NewStringUTF(report);
}

// ─────────────────────────────────────────────────────────────
// Device identity / crypto helpers
// ─────────────────────────────────────────────────────────────

JNIEXPORT jstring JNICALL
Java_com_apex_root_data_jni_NativeBridge_getDeviceIdentifier(JNIEnv* env, jobject) {
    char buf[256];
    // Build device ID from system properties
    auto read_prop = [](const char* prop) -> std::string {
        FILE* f = fopen(prop, "r");
        if (!f) return "";
        char line[128] = {};
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
    return env->NewStringUTF(buf);
}

JNIEXPORT jbyteArray JNICALL
Java_com_apex_root_data_jni_NativeBridge_sha3_512(JNIEnv* env, jobject, jbyteArray data) {
    jsize len = env->GetArrayLength(data);
    jbyte* bytes = env->GetByteArrayElements(data, nullptr);
    auto hash = apex::crypto::sha3_512(
        reinterpret_cast<const uint8_t*>(bytes), static_cast<size_t>(len));
    env->ReleaseByteArrayElements(data, bytes, JNI_ABORT);

    jbyteArray result = env->NewByteArray(64);
    env->SetByteArrayRegion(result, 0, 64, reinterpret_cast<const jbyte*>(hash.data()));
    return result;
}

} // extern "C"
