package com.apex.root.util

import android.content.Context
import android.content.Intent
import android.net.Uri
import androidx.core.content.FileProvider
import com.apex.root.data.FixRecommendation
import com.apex.root.data.FixRecommendations
import com.apex.root.data.jni.NativeBridge
import com.apex.root.viewmodel.trusted.ApexUiState
import java.io.File
import java.text.SimpleDateFormat
import java.util.*

object ReportExporter {

    data class ExportReport(
        val appName: String = "APEX Root",
        val version: String = "1.0.2",
        val timestamp: String = SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault()).format(Date()),
        val riskScore: Int = 0,
        val scanResult: String = "",
        val deepReport: String = "",
        val memFingerprintMask: Int = 0,
        val rwxPageCount: Int = 0,
        val hasShamiko: Boolean = false,
        val hasZygiskNext: Boolean = false,
        val selinuxCompromised: Boolean = false,
        val selfCheckIssues: List<String> = emptyList(),
        val recommendations: List<RecommendationExport> = emptyList()
    )

    data class RecommendationExport(
        val title: String,
        val description: String,
        val steps: List<String>,
        val priority: Int
    )

    fun buildReport(uiState: ApexUiState): ExportReport {
        val scanLayers = parseScanLayers(uiState.scanResult)
        val recs = matchRecommendations(scanLayers)

        return ExportReport(
            riskScore = uiState.riskScore,
            scanResult = uiState.scanResult,
            deepReport = uiState.deepReport,
            memFingerprintMask = uiState.memFingerprintMask,
            rwxPageCount = uiState.rwxPageCount,
            hasShamiko = uiState.hasShamiko,
            hasZygiskNext = uiState.hasZygiskNext,
            selinuxCompromised = uiState.selinuxCompromised,
            selfCheckIssues = uiState.selfCheckIssues,
            recommendations = recs.map {
                RecommendationExport(
                    title = it.titleZh,
                    description = it.descriptionZh,
                    steps = it.stepsZh,
                    priority = it.priority
                )
            }
        )
    }

    fun exportToJson(uiState: ApexUiState): String {
        val report = buildReport(uiState)
        val sb = StringBuilder()
        sb.appendLine("{")
        sb.appendLine("  \"appName\": \"${report.appName}\",")
        sb.appendLine("  \"version\": \"${report.version}\",")
        sb.appendLine("  \"timestamp\": \"${report.timestamp}\",")
        sb.appendLine("  \"riskScore\": ${report.riskScore},")
        sb.appendLine("  \"scanResult\": ${jsonEscape(report.scanResult)},")
        sb.appendLine("  \"deepReport\": ${jsonEscape(report.deepReport)},")
        sb.appendLine("  \"memFingerprintMask\": ${report.memFingerprintMask},")
        sb.appendLine("  \"rwxPageCount\": ${report.rwxPageCount},")
        sb.appendLine("  \"hasShamiko\": ${report.hasShamiko},")
        sb.appendLine("  \"hasZygiskNext\": ${report.hasZygiskNext},")
        sb.appendLine("  \"selinuxCompromised\": ${report.selinuxCompromised},")
        sb.appendLine("  \"selfCheckIssues\": [")
        report.selfCheckIssues.forEachIndexed { i, issue ->
            sb.append("    ${jsonEscape(issue)}")
            if (i < report.selfCheckIssues.lastIndex) sb.append(",")
            sb.appendLine()
        }
        sb.appendLine("  ],")
        sb.appendLine("  \"recommendations\": [")
        report.recommendations.forEachIndexed { i, rec ->
            sb.appendLine("    {")
            sb.appendLine("      \"title\": ${jsonEscape(rec.title)},")
            sb.appendLine("      \"description\": ${jsonEscape(rec.description)},")
            sb.appendLine("      \"priority\": ${rec.priority},")
            sb.appendLine("      \"steps\": [")
            rec.steps.forEachIndexed { j, step ->
                sb.append("        ${jsonEscape(step)}")
                if (j < rec.steps.lastIndex) sb.append(",")
                sb.appendLine()
            }
            sb.appendLine("      ]")
            sb.append("    }")
            if (i < report.recommendations.lastIndex) sb.append(",")
            sb.appendLine()
        }
        sb.appendLine("  ]")
        sb.appendLine("}")
        return sb.toString()
    }

    fun shareReport(context: Context, uiState: ApexUiState) {
        val json = exportToJson(uiState)
        val file = File(context.cacheDir, "apex_report_${System.currentTimeMillis()}.json")
        file.writeText(json)

        val uri = FileProvider.getUriForFile(
            context,
            "${context.packageName}.fileprovider",
            file
        )

        val intent = Intent(Intent.ACTION_SEND).apply {
            type = "application/json"
            putExtra(Intent.EXTRA_STREAM, uri)
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
        }
        context.startActivity(Intent.createChooser(intent, "导出检测报告"))
    }

    private fun parseScanLayers(result: String): List<String> {
        val layers = mutableListOf<String>()
        result.lines().forEach { line ->
            when {
                line.contains("系统属性") -> layers.add("属性")
                line.contains("ART") -> layers.add("内存")
                line.contains("内存特征") -> layers.add("内存")
                line.contains("挂载") -> layers.add("挂载")
                line.contains("侧信道") -> layers.add("系统调用时序")
                line.contains("内核") && line.contains("完整性") -> layers.add("内核")
                line.contains("Boot") -> layers.add("固件")
                line.contains("Magisk") -> layers.add("文件")
                line.contains("KernelSU") -> layers.add("内核模块")
                line.contains("APatch") -> layers.add("APatch")
                line.contains("Hook") || line.contains("Xposed") -> layers.add("自保护")
                line.contains("ROM") -> layers.add("固件完整性")
            }
        }
        return layers.distinct()
    }

    private fun matchRecommendations(layers: List<String>): List<FixRecommendation> {
        return FixRecommendations.getRecommendationsForLayers(layers)
    }

    private fun jsonEscape(s: String): String {
        val sb = StringBuilder()
        sb.append('"')
        for (c in s) {
            when (c) {
                '"' -> sb.append("\\\"")
                '\\' -> sb.append("\\\\")
                '\n' -> sb.append("\\n")
                '\r' -> sb.append("\\r")
                '\t' -> sb.append("\\t")
                else -> sb.append(c)
            }
        }
        sb.append('"')
        return sb.toString()
    }
}
