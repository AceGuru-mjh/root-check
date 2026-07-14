package com.apex.root.domain.trust.model

data class ScanTask(
    val taskId: String,
    val level: DetectionLevel,
    val enabledServices: List<String>,
    val nonce: ByteArray,
    val timestamp: Long
)

enum class DetectionLevel {
    QUICK,
    STANDARD,
    DEEP,
    FORENSIC
}
