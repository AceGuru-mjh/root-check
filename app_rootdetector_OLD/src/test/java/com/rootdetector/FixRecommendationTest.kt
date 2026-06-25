package com.rootdetector

import com.rootdetector.data.FixRecommendations
import com.rootdetector.detect.DetectionResult
import com.rootdetector.util.FixRecommendationGenerator
import org.junit.Assert.*
import org.junit.Test
import java.util.Locale

class FixRecommendationTest {

    @Test
    fun testGetRecommendationByLayer() {
        val recommendation = FixRecommendations.getRecommendation("属性检测")
        assertNotNull(recommendation)
        assertEquals("properties", recommendation?.id)
        assertEquals("系统属性异常", recommendation?.titleZh)
    }

    @Test
    fun testGetRecommendationByLayerPartialMatch() {
        val recommendation = FixRecommendations.getRecommendation("文件")
        assertNotNull(recommendation)
        assertEquals("files", recommendation?.id)
    }

    @Test
    fun testGetRecommendationNotFound() {
        val recommendation = FixRecommendations.getRecommendation("未知检测项")
        assertNull(recommendation)
    }

    @Test
    fun testGenerateRecommendationsFromDetectedResults() {
        val results = listOf(
            DetectionResult(detected = true, layer = "属性检测", detail = "ro.debuggable=1"),
            DetectionResult(detected = false, layer = "文件检测", detail = ""),
            DetectionResult(detected = true, layer = "内存检测", detail = "su binary found")
        )

        val recommendations = FixRecommendationGenerator.generateRecommendations(results, Locale.CHINESE)
        
        assertEquals(2, recommendations.size)
        assertEquals("properties", recommendations[0].id)
        assertEquals("memory", recommendations[1].id)
    }

    @Test
    fun testGenerateRecommendationsSortedByPriority() {
        val results = listOf(
            DetectionResult(detected = true, layer = "内存检测", detail = ""),
            DetectionResult(detected = true, layer = "文件检测", detail = ""),
            DetectionResult(detected = true, layer = "APatch检测", detail = "")
        )

        val recommendations = FixRecommendationGenerator.generateRecommendations(results, Locale.CHINESE)
        
        assertEquals(3, recommendations.size)
        // APatch (priority 10) should be first
        assertEquals("apatch", recommendations[0].id)
        // 文件 (priority 9) should be second
        assertEquals("files", recommendations[1].id)
        // 内存 (priority 7) should be third
        assertEquals("memory", recommendations[2].id)
    }

    @Test
    fun testGenerateRecommendationsNoDetection() {
        val results = listOf(
            DetectionResult(detected = false, layer = "属性检测", detail = ""),
            DetectionResult(detected = false, layer = "文件检测", detail = "")
        )

        val recommendations = FixRecommendationGenerator.generateRecommendations(results, Locale.CHINESE)
        
        assertTrue(recommendations.isEmpty())
    }

    @Test
    fun testFormatRecommendationTextChinese() {
        val results = listOf(
            DetectionResult(detected = true, layer = "属性检测", detail = "")
        )

        val recommendations = FixRecommendationGenerator.generateRecommendations(results, Locale.CHINESE)
        val text = FixRecommendationGenerator.formatRecommendationText(recommendations, Locale.CHINESE)
        
        assertTrue(text.contains("修复建议"))
        assertTrue(text.contains("系统属性异常"))
        assertTrue(text.contains("修复步骤"))
    }

    @Test
    fun testFormatRecommendationTextEnglish() {
        val results = listOf(
            DetectionResult(detected = true, layer = "属性检测", detail = "")
        )

        val recommendations = FixRecommendationGenerator.generateRecommendations(results, Locale.ENGLISH)
        val text = FixRecommendationGenerator.formatRecommendationText(recommendations, Locale.ENGLISH)
        
        assertTrue(text.contains("Fix Recommendations"))
        assertTrue(text.contains("System Properties Abnormal"))
        assertTrue(text.contains("Fix Steps"))
    }

    @Test
    fun testFormatRecommendationTextEmpty() {
        val recommendations = emptyList<com.rootdetector.data.FixRecommendation>()
        
        val textZh = FixRecommendationGenerator.formatRecommendationText(recommendations, Locale.CHINESE)
        assertTrue(textZh.contains("设备环境安全"))
        
        val textEn = FixRecommendationGenerator.formatRecommendationText(recommendations, Locale.ENGLISH)
        assertTrue(textEn.contains("Device environment is safe"))
    }

    @Test
    fun testGetRecommendationSummaryCritical() {
        val results = listOf(
            DetectionResult(detected = true, layer = "APatch检测", detail = ""),
            DetectionResult(detected = true, layer = "内核检测", detail = "")
        )

        val recommendations = FixRecommendationGenerator.generateRecommendations(results, Locale.CHINESE)
        val summary = FixRecommendationGenerator.getRecommendationSummary(recommendations, Locale.CHINESE)
        
        assertEquals("严重", summary)
    }

    @Test
    fun testGetRecommendationSummaryHigh() {
        val results = listOf(
            DetectionResult(detected = true, layer = "文件检测", detail = ""),
            DetectionResult(detected = true, layer = "自保护检测", detail = "")
        )

        val recommendations = FixRecommendationGenerator.generateRecommendations(results, Locale.CHINESE)
        val summary = FixRecommendationGenerator.getRecommendationSummary(recommendations, Locale.CHINESE)
        
        assertEquals("高", summary)
    }

    @Test
    fun testGetRecommendationSummarySafe() {
        val recommendations = emptyList<com.rootdetector.data.FixRecommendation>()
        val summary = FixRecommendationGenerator.getRecommendationSummary(recommendations, Locale.CHINESE)
        
        assertEquals("安全", summary)
    }

    @Test
    fun testCopyStepsToClipboardChinese() {
        val recommendation = FixRecommendations.getRecommendation("属性检测")!!
        val steps = FixRecommendationGenerator.copyStepsToClipboard(recommendation, Locale.CHINESE)
        
        assertTrue(steps.contains("打开终端模拟器"))
        assertTrue(steps.contains("getprop"))
    }

    @Test
    fun testCopyStepsToClipboardEnglish() {
        val recommendation = FixRecommendations.getRecommendation("属性检测")!!
        val steps = FixRecommendationGenerator.copyStepsToClipboard(recommendation, Locale.ENGLISH)
        
        assertTrue(steps.contains("Open terminal emulator"))
        assertTrue(steps.contains("getprop"))
    }

    @Test
    fun testAllRecommendationsHaveValidData() {
        val allRecommendations = FixRecommendations.getAllRecommendations()
        
        allRecommendations.forEach { rec ->
            assertTrue("Title ZH should not be empty for ${rec.id}", rec.titleZh.isNotEmpty())
            assertTrue("Title EN should not be empty for ${rec.id}", rec.titleEn.isNotEmpty())
            assertTrue("Description ZH should not be empty for ${rec.id}", rec.descriptionZh.isNotEmpty())
            assertTrue("Description EN should not be empty for ${rec.id}", rec.descriptionEn.isNotEmpty())
            assertTrue("Steps ZH should not be empty for ${rec.id}", rec.stepsZh.isNotEmpty())
            assertTrue("Steps EN should not be empty for ${rec.id}", rec.stepsEn.isNotEmpty())
            assertTrue("Priority should be positive for ${rec.id}", rec.priority > 0)
        }
    }

    @Test
    fun testRecommendationsDeduplication() {
        val results = listOf(
            DetectionResult(detected = true, layer = "属性检测", detail = ""),
            DetectionResult(detected = true, layer = "属性", detail = "")
        )

        val recommendations = FixRecommendationGenerator.generateRecommendations(results, Locale.CHINESE)
        
        // Should only have one recommendation for "属性" even though it appears twice
        assertEquals(1, recommendations.size)
    }
}
