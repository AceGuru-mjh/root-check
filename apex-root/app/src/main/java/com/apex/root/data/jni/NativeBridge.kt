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

    fun detectSyscallTableHook(): Boolean = NativeLibraryLoader.safeCall(false) {
        detectSyscallTableHookNative()
    }

    // ─── ART enhanced detection ───────────────────────
    fun artEnhancedScan(): String = NativeLibraryLoader.safeCall("") {
        artEnhancedScanNative()
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
    private external fun detectSyscallTableHookNative(): Boolean
    private external fun artEnhancedScanNative(): String
    private external fun detectFirmwareTamperingNative(): Boolean
    private external fun detectAVBStatusNative(): Boolean
    private external fun detectCustomRecoveryNative(): Boolean
    private external fun firmwareFullScanNative(): String
    private external fun getDeviceIdentifierNative(): String?
    private external fun sha3_512Native(data: ByteArray): ByteArray
}
