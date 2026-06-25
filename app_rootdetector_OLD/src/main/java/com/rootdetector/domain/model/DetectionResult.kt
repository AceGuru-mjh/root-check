package com.rootdetector.domain.model

data class PluginResult(
    val layerId: Int,
    val detected: Boolean,
    val confidence: Int,
    val riskScore: Int,
    val costNs: Long,
    val detail: String
)

data class DetectionResult(
    val layerResults: List<PluginResult>,
    val rootDetected: Boolean,
    val riskScore: Int,
    val reportId: String
)

enum class ThreatLevel {
    SAFE, LOW, MEDIUM, HIGH, CRITICAL;

    companion object {
        fun fromScore(score: Int) = when {
            score <= 0   -> SAFE
            score <= 20  -> LOW
            score <= 50  -> MEDIUM
            score <= 80  -> HIGH
            else         -> CRITICAL
        }
    }
}