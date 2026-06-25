package com.rootdetector.util

import com.rootdetector.data.FixRecommendation
import com.rootdetector.data.FixRecommendations
import com.rootdetector.detect.DetectionResult
import java.util.Locale

object FixRecommendationGenerator {

    fun generateRecommendations(
        results: List<DetectionResult>,
        locale: Locale = Locale.getDefault()
    ): List<FixRecommendation> {
        val detectedLayers = results.filter { it.detected }.map { it.layer }
        return FixRecommendations.getRecommendationsForLayers(detectedLayers)
    }

    fun formatRecommendationText(
        recommendations: List<FixRecommendation>,
        locale: Locale = Locale.getDefault()
    ): String {
        if (recommendations.isEmpty()) {
            return if (locale.language == "zh") {
                "✓ 设备环境安全，未发现需要修复的问题"
            } else {
                "✓ Device environment is safe, no issues need to be fixed"
            }
        }

        return buildString {
            if (locale.language == "zh") {
                appendLine("=== 修复建议 ===")
                appendLine()
                recommendations.forEachIndexed { index, rec ->
                    appendLine("${index + 1}. ${rec.titleZh}")
                    appendLine("   ${rec.descriptionZh}")
                    appendLine("   修复步骤:")
                    rec.stepsZh.forEach { step ->
                        appendLine("   $step")
                    }
                    if (rec.oneClickFixAvailable) {
                        appendLine("   [可一键修复]")
                    }
                    appendLine()
                }
            } else {
                appendLine("=== Fix Recommendations ===")
                appendLine()
                recommendations.forEachIndexed { index, rec ->
                    appendLine("${index + 1}. ${rec.titleEn}")
                    appendLine("   ${rec.descriptionEn}")
                    appendLine("   Fix Steps:")
                    rec.stepsEn.forEach { step ->
                        appendLine("   $step")
                    }
                    if (rec.oneClickFixAvailable) {
                        appendLine("   [One-click fix available]")
                    }
                    appendLine()
                }
            }
        }
    }

    fun getRecommendationSummary(
        recommendations: List<FixRecommendation>,
        locale: Locale = Locale.getDefault()
    ): String {
        if (recommendations.isEmpty()) {
            return if (locale.language == "zh") "安全" else "Safe"
        }

        val maxPriority = recommendations.maxOf { it.priority }
        return when {
            maxPriority >= 9 -> if (locale.language == "zh") "严重" else "Critical"
            maxPriority >= 7 -> if (locale.language == "zh") "高" else "High"
            maxPriority >= 5 -> if (locale.language == "zh") "中" else "Medium"
            else -> if (locale.language == "zh") "低" else "Low"
        }
    }

    fun copyStepsToClipboard(
        recommendation: FixRecommendation,
        locale: Locale = Locale.getDefault()
    ): String {
        val steps = if (locale.language == "zh") recommendation.stepsZh else recommendation.stepsEn
        return steps.joinToString("\n")
    }
}
