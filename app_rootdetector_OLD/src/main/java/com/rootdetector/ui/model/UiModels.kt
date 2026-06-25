package com.rootdetector.ui.model

import com.rootdetector.domain.model.DetectionLevel
import com.rootdetector.domain.model.PluginResult
import com.rootdetector.domain.model.ThreatLevel

sealed interface DetectionUiState {
    data object Idle : DetectionUiState
    data object Running : DetectionUiState
    data class Success(
        val level: DetectionLevel,
        val layerResults: List<PluginResult>,
        val rootDetected: Boolean,
        val riskScore: Int,
        val threatLevel: ThreatLevel,
        val elapsedMs: Long
    ) : DetectionUiState
    data class Error(val message: String) : DetectionUiState
}

data class HistoryEntry(
    val id: String,
    val timestamp: Long,
    val level: DetectionLevel,
    val rootDetected: Boolean,
    val riskScore: Int,
    val layerCount: Int
)

data class SettingsState(
    val darkTheme: Boolean = true,
    val useDaemon: Boolean = true,
    val cacheResults: Boolean = false,
    val languageZh: Boolean = true
)

fun ThreatLevel.toColorHex(): String = when (this) {
    ThreatLevel.SAFE -> "#4CAF50"
    ThreatLevel.LOW -> "#FFC107"
    ThreatLevel.MEDIUM -> "#FF9800"
    ThreatLevel.HIGH -> "#F44336"
    ThreatLevel.CRITICAL -> "#9C27B0"
}

fun Int.toThreatColorHex(): String = ThreatLevel.fromScore(this).toColorHex()

fun threatLevelLabel(level: ThreatLevel, zh: Boolean): String = when (level) {
    ThreatLevel.SAFE -> if (zh) "安全" else "Safe"
    ThreatLevel.LOW -> if (zh) "低风险" else "Low"
    ThreatLevel.MEDIUM -> if (zh) "中风险" else "Medium"
    ThreatLevel.HIGH -> if (zh) "高风险" else "High"
    ThreatLevel.CRITICAL -> if (zh) "严重" else "Critical"
}
