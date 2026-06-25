package com.apex.root.domain.trust.model

data class TrustedLayerResult(
    val serviceId: String,
    val serviceName: String,
    val success: Boolean,
    val confidence: Float,
    val findings: List<Finding>,
    val rawHash: ByteArray,
    val signatures: List<ByteArray>,
    val durationMs: Long
)

data class Finding(
    val type: FindingType,
    val severity: Severity,
    val description: String,
    val evidence: String
)

enum class FindingType {
    ROOT_BINARY,
    SU_HIDE,
    PROPERTY_TAMPER,
    HOOK_DETECTED,
    MOUNT_OVERLAY,
    KERNEL_ROOTKIT,
    TEE_TAMPER,
    BOOTLOADER_UNLOCKED,
    CUSTOM_ROM,
    DANGEROUS_APP,
    PIF_DETECTED,
    ZYGISK_INJECTION,
    VIRTUALIZATION,
    MEMORY_TAMPER,
    PROCESS_HIDE
}

enum class Severity {
    SAFE,
    INFO,
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
}
