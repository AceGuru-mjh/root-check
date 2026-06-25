package com.apex.root.data.jni

object NativeBridge {
    init {
        System.loadLibrary("apex_root")
    }

    external fun runQuickScan(): String
    external fun isDeviceRooted(): Boolean
    external fun getRiskScore(): Int

    // Post-quantum crypto
    external fun isPostQuantumAvailable(): Boolean
    external fun getSignedReport(): String
    external fun getPostQuantumInfo(): String

    // ─── Deep memory fingerprint scan ───────────────────
    external fun fullMemoryFingerprint(): Int
    external fun deepMemoryScanReport(): String
    external fun countRWXPages(): Int

    // ─── SELinux context detection ─────────────────────
    external fun detectSELinuxContextJump(): Boolean
    external fun detectSELinuxPolicyMod(): Boolean
    external fun selinuxFullScan(): String

    // ─── Anti-hiding probes ────────────────────────────
    external fun detectShamiko(): Boolean
    external fun shamikoFullScan(): String
    external fun detectZygiskNext(): Boolean
    external fun zygiskNextFullScan(): String
    external fun detectProcessHiding(): Boolean
    external fun detectMountNamespaceHiding(): Boolean
    external fun detectSyscallTableHook(): Boolean

    // ─── ART enhanced detection ───────────────────────
    external fun artEnhancedScan(): String

    // ─── Firmware partition integrity ─────────────────
    external fun detectFirmwareTampering(): Boolean
    external fun detectAVBStatus(): Boolean
    external fun detectCustomRecovery(): Boolean
    external fun firmwareFullScan(): String

    // ─── Device identity / crypto helpers ────────────
    external fun getDeviceIdentifier(): String?
    external fun sha3_512(data: ByteArray): ByteArray
}
