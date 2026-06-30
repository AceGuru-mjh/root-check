package com.apex.root.data.jni

import com.apex.root.core.NativeLibraryLoader

/**
 * JNI 桥接层 — 所有 native 方法调用都通过 NativeLibraryLoader 安全保护。
 * 避免 UnsatisfiedLinkError 直接崩溃。
 */
object NativeBridge {

    // ─── Quick Scan ────────────────────────────────────
    fun runQuickScan(): String = NativeLibraryLoader.safeCall("扫描不可用：原生库未加载") {
        runQuickScanNative()
    }

    fun isDeviceRooted(): Boolean = NativeLibraryLoader.safeCall(false) {
        isDeviceRootedNative()
    }

    fun getRiskScore(): Int = NativeLibraryLoader.safeCall(0) {
        getRiskScoreNative()
    }

    // ─── Post-quantum crypto ──────────────────────────
    fun isPostQuantumAvailable(): Boolean = NativeLibraryLoader.safeCall(false) {
        isPostQuantumAvailableNative()
    }

    fun getSignedReport(): String = NativeLibraryLoader.safeCall("") {
        getSignedReportNative()
    }

    fun getPostQuantumInfo(): String = NativeLibraryLoader.safeCall("") {
        getPostQuantumInfoNative()
    }

    // ─── Deep memory fingerprint scan ───────────────────
    fun fullMemoryFingerprint(): Int = NativeLibraryLoader.safeCall(0) {
        fullMemoryFingerprintNative()
    }

    fun deepMemoryScanReport(): String = NativeLibraryLoader.safeCall("") {
        deepMemoryScanReportNative()
    }

    fun countRWXPages(): Int = NativeLibraryLoader.safeCall(0) {
        countRWXPagesNative()
    }

    // ─── SELinux context detection ─────────────────────
    fun detectSELinuxContextJump(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectSELinuxContextJumpNative()
    }

    fun detectSELinuxPolicyMod(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectSELinuxPolicyModNative()
    }

    fun selinuxFullScan(): String = NativeLibraryLoader.safeCall("") {
        selinuxFullScanNative()
    }

    // ─── Anti-hiding probes ────────────────────────────
    fun detectShamiko(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectShamikoNative()
    }

    fun shamikoFullScan(): String = NativeLibraryLoader.safeCall("") {
        shamikoFullScanNative()
    }

    fun detectZygiskNext(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectZygiskNextNative()
    }

    fun zygiskNextFullScan(): String = NativeLibraryLoader.safeCall("") {
        zygiskNextFullScanNative()
    }

    fun detectProcessHiding(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectProcessHidingNative()
    }

    fun detectMountNamespaceHiding(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectMountNamespaceHidingNative()
    }

    // 已移除：detectSyscallTableHook() — Ring0 检测，依赖 /proc/kallsyms
    // 改用：detectSyscallResultInconsistency() — 用户态 syscall 结果一致性检测
    fun detectSyscallResultInconsistency(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectSyscallResultInconsistencyNative()
    }

    // ─── ART enhanced detection ───────────────────────
    fun artEnhancedScan(): String = NativeLibraryLoader.safeCall("") {
        artEnhancedScanNative()
    }

    // ─── L14: VirtualXposed / 太极 / 双开分身 ────────────
    fun detectVirtualXposed(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectVirtualXposedNative()
    }

    fun detectTaiChi(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectTaiChiNative()
    }

    fun detectDualSpaceApps(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectDualSpaceAppsNative()
    }

    fun virtualXposedFullScan(): String = NativeLibraryLoader.safeCall("") {
        virtualXposedFullScanNative()
    }

    // ─── L15: 危险应用检测 ────────────────────────────
    fun detectGameGuardian(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectGameGuardianNative()
    }

    fun detectCheatEngine(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectCheatEngineNative()
    }

    fun detectLuckyPatcher(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectLuckyPatcherNative()
    }

    fun detectMemoryEditors(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectMemoryEditorsNative()
    }

    fun detectCrackingTools(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectCrackingToolsNative()
    }

    fun dangerousAppsFullScan(): String = NativeLibraryLoader.safeCall("") {
        dangerousAppsFullScanNative()
    }

    // ─── L16: Magisk 扩展检测 ────────────────────────
    fun detectMagiskDenyList(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectMagiskDenyListNative()
    }

    fun detectZygiskModules(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectZygiskModulesNative()
    }

    fun detectLSPosedManager(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectLSPosedManagerNative()
    }

    fun detectRiruModules(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectRiruModulesNative()
    }

    fun detectModernForks(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectModernForksNative()
    }

    fun detectHideMyApplist(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectHideMyApplistNative()
    }

    fun detectStorageIsolation(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectStorageIsolationNative()
    }

    fun detectMagiskHideLegacy(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectMagiskHideLegacyNative()
    }

    fun magiskExtensionsFullScan(): String = NativeLibraryLoader.safeCall("") {
        magiskExtensionsFullScanNative()
    }

    // ─── Firmware partition integrity ─────────────────
    fun detectFirmwareTampering(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectFirmwareTamperingNative()
    }

    fun detectAVBStatus(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectAVBStatusNative()
    }

    fun detectCustomRecovery(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectCustomRecoveryNative()
    }

    fun firmwareFullScan(): String = NativeLibraryLoader.safeCall("") {
        firmwareFullScanNative()
    }

    // ─── Device identity / crypto helpers ────────────
    fun getDeviceIdentifier(): String? = NativeLibraryLoader.safeCall(null) {
        getDeviceIdentifierNative()
    }

    fun sha3_512(data: ByteArray): ByteArray = NativeLibraryLoader.safeCall(ByteArray(0)) {
        sha3_512Native(data)
    }

    // ─── 原始 external 方法（私有）─────────────────────
    private external fun runQuickScanNative(): String
    private external fun isDeviceRootedNative(): Boolean
    private external fun getRiskScoreNative(): Int
    private external fun isPostQuantumAvailableNative(): Boolean
    private external fun getSignedReportNative(): String
    private external fun getPostQuantumInfoNative(): String
    private external fun fullMemoryFingerprintNative(): Int
    private external fun deepMemoryScanReportNative(): String
    private external fun countRWXPagesNative(): Int
    private external fun detectSELinuxContextJumpNative(): Boolean
    private external fun detectSELinuxPolicyModNative(): Boolean
    private external fun selinuxFullScanNative(): String
    private external fun detectShamikoNative(): Boolean
    private external fun shamikoFullScanNative(): String
    private external fun detectZygiskNextNative(): Boolean
    private external fun zygiskNextFullScanNative(): String
    private external fun detectProcessHidingNative(): Boolean
    private external fun detectMountNamespaceHidingNative(): Boolean
    // 已移除：detectSyscallTableHookNative — Ring0 检测
    private external fun detectSyscallResultInconsistencyNative(): Boolean
    private external fun artEnhancedScanNative(): String
    // L14 / L15 / L16 native
    private external fun detectVirtualXposedNative(): Boolean
    private external fun detectTaiChiNative(): Boolean
    private external fun detectDualSpaceAppsNative(): Boolean
    private external fun virtualXposedFullScanNative(): String
    private external fun detectGameGuardianNative(): Boolean
    private external fun detectCheatEngineNative(): Boolean
    private external fun detectLuckyPatcherNative(): Boolean
    private external fun detectMemoryEditorsNative(): Boolean
    private external fun detectCrackingToolsNative(): Boolean
    private external fun dangerousAppsFullScanNative(): String
    private external fun detectMagiskDenyListNative(): Boolean
    private external fun detectZygiskModulesNative(): Boolean
    private external fun detectLSPosedManagerNative(): Boolean
    private external fun detectRiruModulesNative(): Boolean
    private external fun detectModernForksNative(): Boolean
    private external fun detectHideMyApplistNative(): Boolean
    private external fun detectStorageIsolationNative(): Boolean
    private external fun detectMagiskHideLegacyNative(): Boolean
    private external fun magiskExtensionsFullScanNative(): String
    private external fun detectFirmwareTamperingNative(): Boolean
    private external fun detectAVBStatusNative(): Boolean
    private external fun detectCustomRecoveryNative(): Boolean
    private external fun firmwareFullScanNative(): String
    private external fun getDeviceIdentifierNative(): String?
    private external fun sha3_512Native(data: ByteArray): ByteArray
}
