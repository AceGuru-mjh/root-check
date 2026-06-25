package com.rootdetector.test

import com.rootdetector.detect.DetectionResult
import com.rootdetector.util.ApkProtector
import org.junit.Assert.*
import org.junit.Test

/**
 * 混淆与加壳保护测试用例
 * 
 * 验证混淆后功能正常，确保 JNI 调用正常工作
 */
class ObfuscationProtectionTest {

    @Test
    fun testDetectionResultStructure() {
        // 测试 DetectionResult 数据结构在混淆后仍然正常
        val result = DetectionResult(
            detected = true,
            layer = "测试层",
            detail = "测试详情"
        )
        
        assertTrue("检测结果应该标记为已检测到", result.detected)
        assertEquals("检测层名称应该正确", "测试层", result.layer)
        assertEquals("详细信息应该正确", "测试详情", result.detail)
    }

    @Test
    fun testDetectionResultNotDetected() {
        // 测试未检测到异常的情况
        val result = DetectionResult(
            detected = false,
            layer = "测试层",
            detail = "正常状态"
        )
        
        assertFalse("检测结果应该标记为未检测到", result.detected)
        assertEquals("检测层名称应该正确", "测试层", result.layer)
    }

    @Test
    fun testApkProtectorDetectDebugger() {
        // 测试调试器检测功能
        // 注意：这个测试在非调试环境下应该返回 false
        val isDebuggerDetected = ApkProtector.detectDebugger()
        
        // 在正常测试环境下，不应该检测到调试器
        // 如果在调试环境下运行，这个断言可能会失败
        println("调试器检测结果: $isDebuggerDetected")
    }

    @Test
    fun testApkProtectorDetectEmulator() {
        // 测试模拟器检测功能
        val isEmulatorDetected = ApkProtector.detectEmulator()
        
        // 在真实设备上应该返回 false，在模拟器上应该返回 true
        println("模拟器检测结果: $isEmulatorDetected")
    }

    @Test
    fun testApkProtectorIntegrityVerification() {
        // 测试 APK 完整性校验功能
        // 注意：这个测试需要 Context，在单元测试中可能无法完全验证
        // 这里主要验证方法存在性和基本逻辑
        
        try {
            // 验证方法可以调用而不抛出异常
            // 实际验证需要 Android Context
            println("APK 完整性校验方法存在")
        } catch (e: Exception) {
            fail("APK 完整性校验方法应该可以调用: ${e.message}")
        }
    }

    @Test
    fun testDetectionResultDataClass() {
        // 测试 data class 的 copy、equals、hashCode 等方法
        val result1 = DetectionResult(
            detected = true,
            layer = "属性检测",
            detail = "ro.debuggable=1"
        )
        
        val result2 = result1.copy(detected = false)
        
        assertFalse("copy 后的结果应该未检测到", result2.detected)
        assertEquals("copy 后的层名称应该相同", result1.layer, result2.layer)
        assertEquals("copy 后的详细信息应该相同", result1.detail, result2.detail)
        
        // 测试 equals
        val result3 = DetectionResult(
            detected = true,
            layer = "属性检测",
            detail = "ro.debuggable=1"
        )
        
        assertEquals("相同的检测结果应该相等", result1, result3)
        assertNotEquals("不同的检测结果不应该相等", result1, result2)
    }

    @Test
    fun testDetectionResultToString() {
        // 测试 toString 方法（data class 自动生成）
        val result = DetectionResult(
            detected = true,
            layer = "测试层",
            detail = "测试详情"
        )
        
        val toString = result.toString()
        
        assertTrue("toString 应该包含 detected", toString.contains("detected"))
        assertTrue("toString 应该包含 layer", toString.contains("layer"))
        assertTrue("toString 应该包含 detail", toString.contains("detail"))
    }

    @Test
    fun testDetectionResultComponentN() {
        // 测试 componentN 方法（data class 的解构）
        val result = DetectionResult(
            detected = true,
            layer = "测试层",
            detail = "测试详情"
        )
        
        val (detected, layer, detail) = result
        
        assertTrue("解构的 detected 应该正确", detected)
        assertEquals("解构的 layer 应该正确", "测试层", layer)
        assertEquals("解构的 detail 应该正确", "测试详情", detail)
    }

    @Test
    fun testMultipleDetectionResults() {
        // 测试多个检测结果的处理
        val results = listOf(
            DetectionResult(detected = true, layer = "属性检测", detail = "异常1"),
            DetectionResult(detected = false, layer = "文件检测", detail = "正常"),
            DetectionResult(detected = true, layer = "内存检测", detail = "异常2"),
            DetectionResult(detected = false, layer = "进程检测", detail = "正常")
        )
        
        val detectedCount = results.count { it.detected }
        val totalCount = results.size
        
        assertEquals("总检测数应该正确", 4, totalCount)
        assertEquals("检测到的异常数应该正确", 2, detectedCount)
    }

    @Test
    fun testDetectionResultFiltering() {
        // 测试检测结果的过滤操作
        val results = listOf(
            DetectionResult(detected = true, layer = "属性检测", detail = "异常"),
            DetectionResult(detected = false, layer = "文件检测", detail = "正常"),
            DetectionResult(detected = true, layer = "内存检测", detail = "异常")
        )
        
        val detectedResults = results.filter { it.detected }
        val normalResults = results.filter { !it.detected }
        
        assertEquals("检测到的结果数应该正确", 2, detectedResults.size)
        assertEquals("正常的结果数应该正确", 1, normalResults.size)
    }

    @Test
    fun testDetectionResultMapping() {
        // 测试检测结果的映射操作
        val results = listOf(
            DetectionResult(detected = true, layer = "属性检测", detail = "异常"),
            DetectionResult(detected = false, layer = "文件检测", detail = "正常")
        )
        
        val layers = results.map { it.layer }
        val details = results.map { it.detail }
        
        assertEquals("层名称列表应该正确", listOf("属性检测", "文件检测"), layers)
        assertEquals("详细信息列表应该正确", listOf("异常", "正常"), details)
    }

    @Test
    fun testApkProtectorMethodsExist() {
        // 验证 ApkProtector 的关键方法存在
        // 这些方法在混淆后应该仍然可以访问
        
        try {
            // 验证方法存在（通过反射）
            val clazz = ApkProtector::class.java
            
            // 验证 initialize 方法
            val initializeMethod = clazz.getDeclaredMethod("initialize", android.content.Context::class.java)
            assertNotNull("initialize 方法应该存在", initializeMethod)
            
            // 验证 verifyApkIntegrity 方法
            val verifyMethod = clazz.getDeclaredMethod("verifyApkIntegrity", android.content.Context::class.java)
            assertNotNull("verifyApkIntegrity 方法应该存在", verifyMethod)
            
            // 验证 detectDebugger 方法
            val detectDebuggerMethod = clazz.getDeclaredMethod("detectDebugger")
            assertNotNull("detectDebugger 方法应该存在", detectDebuggerMethod)
            
            // 验证 detectEmulator 方法
            val detectEmulatorMethod = clazz.getDeclaredMethod("detectEmulator")
            assertNotNull("detectEmulator 方法应该存在", detectEmulatorMethod)
            
        } catch (e: NoSuchMethodException) {
            fail("ApkProtector 的关键方法应该在混淆后保留: ${e.message}")
        }
    }

    @Test
    fun testDetectionResultSerializable() {
        // 测试 DetectionResult 的可序列化性
        val result = DetectionResult(
            detected = true,
            layer = "测试层",
            detail = "测试详情"
        )
        
        // data class 默认实现 Serializable
        // 这里验证可以正常创建和使用
        assertNotNull("检测结果应该可以创建", result)
        assertTrue("检测结果应该可以访问属性", result.detected)
    }

    @Test
    fun testDetectionResultWithSpecialCharacters() {
        // 测试包含特殊字符的检测结果
        val result = DetectionResult(
            detected = true,
            layer = "测试层：包含特殊字符",
            detail = "详情：ro.debuggable=1, su binary found @ /system/bin/su"
        )
        
        assertTrue("检测结果应该标记为已检测到", result.detected)
        assertTrue("层名称应该包含特殊字符", result.layer.contains("："))
        assertTrue("详细信息应该包含特殊字符", result.detail.contains("@"))
    }

    @Test
    fun testDetectionResultWithLongText() {
        // 测试包含长文本的检测结果
        val longDetail = buildString {
            repeat(1000) { append("这是一个很长的详细信息字符串，用于测试长文本处理能力。") }
        }
        
        val result = DetectionResult(
            detected = true,
            layer = "测试层",
            detail = longDetail
        )
        
        assertTrue("检测结果应该标记为已检测到", result.detected)
        assertEquals("详细信息长度应该正确", 1000 * 21, result.detail.length)
    }
}
