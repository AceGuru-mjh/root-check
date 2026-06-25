package com.rootdetector.util

import com.rootdetector.detect.DetectionResult
import com.rootdetector.model.DetectionReport
import com.rootdetector.model.ReportSummary
import com.rootdetector.model.ThreatLevel

object ReportGenerator {
    
    fun generateReport(
        mode: String,
        results: List<DetectionResult>
    ): DetectionReport {
        val summary = generateSummary(results)
        return DetectionReport(
            timestamp = System.currentTimeMillis(),
            mode = mode,
            results = results,
            summary = summary
        )
    }
    
    private fun generateSummary(results: List<DetectionResult>): ReportSummary {
        val detectedCount = results.count { it.detected }
        val totalCount = results.size
        
        val threatLevel = when {
            detectedCount == 0 -> ThreatLevel.SAFE
            detectedCount <= 2 -> ThreatLevel.LOW
            detectedCount <= 4 -> ThreatLevel.MEDIUM
            detectedCount <= 6 -> ThreatLevel.HIGH
            else -> ThreatLevel.CRITICAL
        }
        
        val recommendations = generateRecommendations(results)
        
        return ReportSummary(threatLevel, recommendations)
    }
    
    private fun generateRecommendations(results: List<DetectionResult>): List<String> {
        val recommendations = mutableListOf<String>()
        
        results.filter { it.detected }.forEach { result ->
            when {
                result.layer.contains("属性") -> {
                    recommendations.add("检测到可疑系统属性，建议检查是否安装了 Root 工具")
                }
                result.layer.contains("文件") -> {
                    recommendations.add("检测到 Root 相关文件，建议卸载 Root 工具或恢复官方系统")
                }
                result.layer.contains("内存") -> {
                    recommendations.add("检测到可疑内存特征，建议重启设备并检查是否有 Root 工具运行")
                }
                result.layer.contains("挂载") -> {
                    recommendations.add("检测到挂载命名空间异常，可能存在进程级隐藏")
                }
                result.layer.contains("APatch") -> {
                    recommendations.add("检测到 APatch 特征，这是一种内核级 Root 工具，建议恢复官方内核")
                }
                result.layer.contains("SELinux") -> {
                    recommendations.add("检测到 SELinux 上下文异常，建议检查 SELinux 策略是否被修改")
                }
                result.layer.contains("内核") -> {
                    recommendations.add("检测到内核异常，建议刷入官方内核或恢复官方系统")
                }
                result.layer.contains("自保护") -> {
                    recommendations.add("检测到自保护机制异常，应用可能被调试或注入")
                }
                result.layer.contains("TEE") || result.layer.contains("固件") -> {
                    recommendations.add("检测到固件分区完整性异常，建议刷入官方固件恢复分区")
                }
            }
        }
        
        if (recommendations.isEmpty()) {
            recommendations.add("设备环境安全，未发现 Root 痕迹")
        }
        
        return recommendations
    }
    
    fun formatReportText(report: DetectionReport): String {
        return buildString {
            appendLine("=== Root 环境检测报告 ===")
            appendLine()
            appendLine("检测时间: ${report.formattedTime}")
            appendLine("检测模式: ${report.mode}")
            appendLine("威胁等级: ${report.summary.threatLevel.displayName}")
            appendLine("检测结果: ${report.detectedCount}/${report.totalCount} 项异常")
            appendLine()
            appendLine("--- 详细结果 ---")
            appendLine()
            
            report.results.forEachIndexed { index, result ->
                appendLine("${index + 1}. ${result.layer}")
                appendLine("   状态: ${if (result.detected) "⚠ 异常" else "✓ 正常"}")
                appendLine("   详情: ${result.detail}")
                appendLine()
            }
            
            appendLine("--- 修复建议 ---")
            appendLine()
            report.summary.recommendations.forEachIndexed { index, rec ->
                appendLine("${index + 1}. $rec")
            }
        }
    }
}
