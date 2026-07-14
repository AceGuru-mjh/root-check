package com.apex.root.domain.model

enum class RiskLevel {
    NONE,
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
}

enum class DetectionLayer {
    L1_PROP,      // System property checks
    L4_BINARY,    // Binary file presence
    L6_NATIVE,    // Native syscall tests
    L8_BEHAVIOR,  // Behavior analysis
    L9_ADVANCED,  // Advanced heuristics
    L10_AI,       // ML-based detection
    L12_DEEP      // Deep kernel inspection
}

enum class ScanStatus {
    IDLE,
    RUNNING,
    PAUSED,
    COMPLETED,
    ERROR
}

data class DeviceInfo(
    val brand: String,
    val model: String,
    val androidVersion: String,
    val sdkInt: Int,
    val securityPatchLevel: String,
    val bootloaderStatus: BootloaderStatus = BootloaderStatus.UNKNOWN,
    val selinuxMode: SELinuxMode = SELinuxMode.UNKNOWN
)

enum class BootloaderStatus {
    UNLOCKED,
    LOCKED,
    UNKNOWN
}

enum class SELinuxMode {
    ENFORCING,
    PERMISSIVE,
    DISABLED,
    UNKNOWN
}

data class RootDetectionResult(
    val isRooted: Boolean,
    val confidence: Float, // 0.0 - 1.0
    val detectedMethods: List<DetectionMethod>,
    val riskLevel: RiskLevel,
    val timestamp: Long = System.currentTimeMillis()
)

data class DetectionMethod(
    val id: String,
    val name: String,
    val layer: DetectionLayer,
    val triggered: Boolean,
    val details: String? = null
)

data class ScanSession(
    val sessionId: String,
    val startTime: Long,
    val endTime: Long? = null,
    val progress: Float = 0f, // 0.0 - 1.0
    val currentLayer: DetectionLayer? = null,
    val result: RootDetectionResult? = null,
    val status: ScanStatus = ScanStatus.IDLE
)

data class GuardEvent(
    val eventId: String,
    val type: GuardEventType,
    val severity: SeverityLevel,
    val description: String,
    val packageName: String? = null,
    val timestamp: Long = System.currentTimeMillis(),
    val handled: Boolean = false
)

enum class GuardEventType {
    ROOT_ATTEMPT,
    SUSPICIOUS_APP,
    SYSTEM_MODIFICATION,
    BOOTLOADER_CHANGE
}

enum class SeverityLevel {
    INFO,
    WARNING,
    ALERT,
    CRITICAL
}
