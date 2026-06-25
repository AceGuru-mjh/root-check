package com.rootdetector.model

import com.rootdetector.detect.DetectionResult
import java.text.SimpleDateFormat
import java.util.*

data class DetectionReport(
    val timestamp: Long,
    val mode: String,
    val results: List<DetectionResult>,
    val summary: ReportSummary
) {
    val formattedTime: String
        get() = SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault()).format(Date(timestamp))
    
    val detectedCount: Int
        get() = results.count { it.detected }
    
    val totalCount: Int
        get() = results.size
    
    val isClean: Boolean
        get() = detectedCount == 0
}

data class ReportSummary(
    val threatLevel: ThreatLevel,
    val recommendations: List<String>
)

enum class ThreatLevel(val displayName: String, val color: String) {
    SAFE("安全", "#4CAF50"),
    LOW("低风险", "#FFC107"),
    MEDIUM("中风险", "#FF9800"),
    HIGH("高风险", "#F44336"),
    CRITICAL("严重", "#9C27B0")
}
