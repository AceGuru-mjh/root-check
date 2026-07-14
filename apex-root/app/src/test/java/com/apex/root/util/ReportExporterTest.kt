package com.apex.root.util

import com.apex.root.data.FixRecommendation
import com.apex.root.viewmodel.trusted.ApexUiState
import org.junit.Assert.*
import org.junit.Test

class ReportExporterTest {

    @Test
    fun `buildReport creates ExportReport from ApexUiState`() {
        val uiState = ApexUiState(
            scanResult = "检测完成\n系统属性: 正常\nART: 异常",
            riskScore = 45,
            memFingerprintMask = 1,
            hasShamiko = true,
            selinuxCompromised = false,
            deepReport = "deep scan data"
        )
        val report = ReportExporter.buildReport(uiState)
        assertEquals(45, report.riskScore)
        assertTrue(report.hasShamiko)
        assertEquals(1, report.memFingerprintMask)
    }

    @Test
    fun `exportToJson produces valid JSON`() {
        val uiState = ApexUiState(scanResult = "OK", riskScore = 10)
        val json = ReportExporter.exportToJson(uiState)
        assertTrue(json.startsWith("{"))
        assertTrue(json.endsWith("}\n"))
        assertTrue(json.contains("\"riskScore\": 10"))
        assertTrue(json.contains("\"appName\": \"APEX Root\""))
        assertTrue(json.contains("\"version\": \"1.0.3\""))
    }

    @Test
    fun `exportToJson includes recommendations when present`() {
        val uiState = ApexUiState(
            scanResult = "检测完成\n系统属性: 检测到异常",
            riskScore = 50,
            recommendations = listOf(
                FixRecommendation(
                    id = "test",
                    titleZh = "测试",
                    titleEn = "Test",
                    descriptionZh = "描述",
                    descriptionEn = "Desc",
                    stepsZh = listOf("步骤1", "步骤2"),
                    stepsEn = listOf("Step 1", "Step 2"),
                    priority = 5
                )
            ),
            showRecommendations = true
        )
        val json = ReportExporter.exportToJson(uiState)
        assertTrue(json.contains("测试"))
        assertTrue(json.contains("步骤1"))
    }

    @Test
    fun `exportToJson handles special characters`() {
        val uiState = ApexUiState(
            scanResult = "line1\n\"quoted\"\t\\tabbed",
            riskScore = 99
        )
        val json = ReportExporter.exportToJson(uiState)
        assertTrue(json.contains("\\n"))
        assertTrue(json.contains("\\\""))
    }

    @Test
    fun `ExportReport has default values`() {
        val report = ReportExporter.ExportReport()
        assertEquals("APEX Root", report.appName)
        assertEquals("1.0.3", report.version)
        assertEquals(0, report.riskScore)
    }
}
