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

    // ─── eBPF capability probe (Hide Mode UX) ─────────
    /**
     * Probe the kernel + SELinux environment for eBPF hide-mode support.
     * Returns a human-readable report describing whether eBPF path will
     * be used or whether hide-mode falls back to mount-namespace + rename.
     */
    fun getEbpfCapabilityReport(): String = NativeLibraryLoader.safeCall("eBPF capability probe unavailable") {
        getEbpfCapabilityReportNative()
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

    // Ring0 /proc/kallsyms-based detectSyscallTableHook() fully removed.
    // Syscall tamper detection is now Ring3-only:
    //   detectSyscallResultInconsistency() — user-mode syscall result consistency.
    fun detectSyscallResultInconsistency(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectSyscallResultInconsistencyNative()
    }

    // ─── ART enhanced detection ───────────────────────
    fun artEnhancedScan(): String = NativeLibraryLoader.safeCall("") {
        artEnhancedScanNative()
    }

    // ─── Xposed 框架检测 ──────────────────────────────
    /** Xposed / LSPosed / EdXposed 框架检测 */
    fun detectXposedFramework(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectXposedFrameworkNative()
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

    // ─── Frida 检测 ────────────────────────────────────
    /** Frida 二进制 / 内存痕迹 / 端口检测（合并布尔结果） */
    fun detectFrida(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectFridaNative()
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

    // ─── Hide Mode (隐藏模式控制) ──────────────────────
    // 启动隐藏模式：对除 APEX 外的应用隐藏 root 痕迹
    fun enableHideMode(appUid: Int): Int = NativeLibraryLoader.safeCall(-1) {
        enableHideModeNative(appUid)
    }

    // 启动游戏模式：aggressive 隐藏 + 性能优化
    fun enableGameMode(appUid: Int): Int = NativeLibraryLoader.safeCall(-1) {
        enableGameModeNative(appUid)
    }

    // 停止隐藏模式，回到 Detection
    fun disableHideMode() = NativeLibraryLoader.safeCall(Unit) {
        disableHideModeNative()
    }

    // 隐藏模式是否激活
    fun isHideModeActive(): Boolean = NativeLibraryLoader.safeCall(false) {
        isHideModeActiveNative()
    }

    // 获取当前模式 (0=Detect / 1=Hide / 2=Game)
    fun getCurrentMode(): Int = NativeLibraryLoader.safeCall(0) {
        getCurrentModeNative()
    }

    // 获取 native 层最近一次错误信息
    fun getLastError(): String = NativeLibraryLoader.safeCall("") {
        getLastErrorNative()
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
    private external fun getEbpfCapabilityReportNative(): String
    private external fun detectSELinuxPolicyModNative(): Boolean
    private external fun selinuxFullScanNative(): String
    private external fun detectShamikoNative(): Boolean
    private external fun shamikoFullScanNative(): String
    private external fun detectZygiskNextNative(): Boolean
    private external fun zygiskNextFullScanNative(): String
    private external fun detectProcessHidingNative(): Boolean
    private external fun detectMountNamespaceHidingNative(): Boolean
    // Ring0 detectSyscallTableHookNative removed — replaced by Ring3 side-channel.
    private external fun detectSyscallResultInconsistencyNative(): Boolean
    private external fun artEnhancedScanNative(): String
    private external fun detectXposedFrameworkNative(): Boolean
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
    private external fun detectFridaNative(): Boolean
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

    // Hide Mode native methods (由 jni/native_bridge.cpp 实现)
    private external fun enableHideModeNative(appUid: Int): Int
    private external fun enableGameModeNative(appUid: Int): Int
    private external fun disableHideModeNative()
    private external fun isHideModeActiveNative(): Boolean
    private external fun getCurrentModeNative(): Int
    private external fun getLastErrorNative(): String
}
