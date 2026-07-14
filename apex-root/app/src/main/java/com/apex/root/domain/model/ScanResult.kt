package com.apex.root.domain.model

data class ScanResult(
    val isRooted: Boolean,
    val riskLevel: RiskLevel,
    val details: String,
    val riskScore: Int = 0,
    val timestamp: Long = System.currentTimeMillis()
)

enum class RiskLevel {
    SAFE, WARNING, DANGER, CRITICAL
}

enum class RootType(val value: Int) {
    NONE(0),
    MAGISK(1),
    KERNELSU(2),
    APATCH(3),
    UNKNOWN(4);

    companion object {
        fun fromValue(v: Int) = entries.firstOrNull { it.value == v } ?: UNKNOWN
    }
}

enum class CureLevel(val value: Int, val label: String) {
    LIGHT(0, "轻度清理"),
    STANDARD(1, "标准修复"),
    DEEP(2, "深度恢复"),
    FACTORY(3, "完全重置")
}

data class CureResult(
    val success: Boolean,
    val rootType: RootType,
    val levelUsed: CureLevel,
    val message: String,
    val needsReboot: Boolean
)

data class GameModeState(
    val active: Boolean,
    val hiddenProcesses: Int = 0
)
