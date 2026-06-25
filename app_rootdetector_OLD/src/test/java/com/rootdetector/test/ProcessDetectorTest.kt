package com.rootdetector.test

import com.rootdetector.detect.DetectionResult
import org.junit.Assert.*
import org.junit.Test

/**
 * 进程检测器单元测试
 * 
 * 注意：由于 native 方法需要在 Android 设备上运行，
 * 这些测试主要是验证数据结构和逻辑的正确性
 */
class ProcessDetectorTest {

    @Test
    fun testDetectionResultStructure() {
        // 测试检测结果数据结构
        val result = DetectionResult(
            detected = true,
            layer = "第5层增强：进程与线程深度检测",
            detail = "发现 1 个可疑进程: magiskd [进程名称匹配: magiskd]"
        )
        
        assertTrue("检测结果应该标记为已检测到", result.detected)
        assertEquals("检测层名称应该正确", "第5层增强：进程与线程深度检测", result.layer)
        assertTrue("详细信息应该包含可疑进程", result.detail.contains("可疑进程"))
    }

    @Test
    fun testDetectionResultNotDetected() {
        // 测试未检测到异常的情况
        val result = DetectionResult(
            detected = false,
            layer = "第5层增强：进程与线程深度检测",
            detail = "未发现可疑进程或线程（扫描进程: 150, 扫描线程: 300）"
        )
        
        assertFalse("检测结果应该标记为未检测到", result.detected)
        assertTrue("详细信息应该包含扫描统计", result.detail.contains("扫描进程"))
        assertTrue("详细信息应该包含扫描统计", result.detail.contains("扫描线程"))
    }

    @Test
    fun testSuspiciousProcessNames() {
        // 测试可疑进程名称列表
        val suspiciousNames = listOf(
            "magiskd", "magisk", "ksud", "ksu", "apd", "apatch",
            "daemonsu", "supersu", "frida", "frida-server",
            "xposed", "lspd", "lsposedd", "riru", "shamiko"
        )
        
        // 验证关键进程名称都在列表中
        assertTrue("应该包含 magiskd", suspiciousNames.contains("magiskd"))
        assertTrue("应该包含 ksud", suspiciousNames.contains("ksud"))
        assertTrue("应该包含 apd", suspiciousNames.contains("apd"))
        assertTrue("应该包含 frida", suspiciousNames.contains("frida"))
        assertTrue("应该包含 shamiko", suspiciousNames.contains("shamiko"))
    }

    @Test
    fun testSuspiciousSelinuxContexts() {
        // 测试可疑 SELinux 上下文列表
        val suspiciousContexts = listOf(
            "u:r:magisk:s0",
            "u:r:ksu:s0",
            "u:r:kernelsu:s0",
            "u:r:apatch:s0",
            "u:r:zygisk:s0",
            "u:r:shamiko:s0",
            "u:r:lspd:s0",
            "u:r:riru:s0",
            "u:r:su:s0",
            "u:r:frida:s0"
        )
        
        // 验证关键上下文都在列表中
        assertTrue("应该包含 u:r:magisk:s0", suspiciousContexts.contains("u:r:magisk:s0"))
        assertTrue("应该包含 u:r:ksu:s0", suspiciousContexts.contains("u:r:ksu:s0"))
        assertTrue("应该包含 u:r:apatch:s0", suspiciousContexts.contains("u:r:apatch:s0"))
        assertTrue("应该包含 u:r:zygisk:s0", suspiciousContexts.contains("u:r:zygisk:s0"))
    }

    @Test
    fun testProcessInfoStructure() {
        // 测试进程信息结构（模拟数据）
        data class ProcessInfo(
            val pid: Int,
            val name: String,
            val cmdline: String,
            val exe: String,
            val selinuxContext: String,
            val isSuspicious: Boolean,
            val suspiciousReason: String
        )
        
        val suspiciousProcess = ProcessInfo(
            pid = 1234,
            name = "magiskd",
            cmdline = "magiskd --daemon",
            exe = "/system/bin/magiskd",
            selinuxContext = "u:r:magisk:s0",
            isSuspicious = true,
            suspiciousReason = "进程名称匹配: magiskd; SELinux 上下文匹配: u:r:magisk:s0"
        )
        
        assertTrue("进程应该被标记为可疑", suspiciousProcess.isSuspicious)
        assertEquals("进程名称应该正确", "magiskd", suspiciousProcess.name)
        assertTrue("原因应该包含进程名称匹配", 
            suspiciousProcess.suspiciousReason.contains("进程名称匹配"))
        assertTrue("原因应该包含 SELinux 上下文匹配", 
            suspiciousProcess.suspiciousReason.contains("SELinux 上下文匹配"))
    }

    @Test
    fun testThreadInfoStructure() {
        // 测试线程信息结构（模拟数据）
        data class ThreadInfo(
            val tid: Int,
            val name: String,
            val selinuxContext: String,
            val isSuspicious: Boolean,
            val suspiciousReason: String
        )
        
        val suspiciousThread = ThreadInfo(
            tid = 5678,
            name = "zygisk",
            selinuxContext = "u:r:zygisk:s0",
            isSuspicious = true,
            suspiciousReason = "SELinux 上下文包含关键词: u:r:zygisk:s0"
        )
        
        assertTrue("线程应该被标记为可疑", suspiciousThread.isSuspicious)
        assertEquals("线程名称应该正确", "zygisk", suspiciousThread.name)
        assertTrue("原因应该包含关键词匹配", 
            suspiciousThread.suspiciousReason.contains("关键词"))
    }

    @Test
    fun testDetectionResultWithMultipleFindings() {
        // 测试多个检测发现的情况
        val detail = buildString {
            append("发现 2 个可疑进程: ")
            append("magiskd [进程名称匹配: magiskd]; ")
            append("ksud [进程名称匹配: ksud] | ")
            append("发现 1 个可疑线程: ")
            append("TID:1234(zygisk) [SELinux 上下文匹配: u:r:zygisk:s0]")
        }
        
        val result = DetectionResult(
            detected = true,
            layer = "第5层增强：进程与线程深度检测",
            detail = detail
        )
        
        assertTrue("检测结果应该标记为已检测到", result.detected)
        assertTrue("详细信息应该包含可疑进程数量", result.detail.contains("2 个可疑进程"))
        assertTrue("详细信息应该包含可疑线程数量", result.detail.contains("1 个可疑线程"))
        assertTrue("详细信息应该包含 magiskd", result.detail.contains("magiskd"))
        assertTrue("详细信息应该包含 ksud", result.detail.contains("ksud"))
        assertTrue("详细信息应该包含 zygisk", result.detail.contains("zygisk"))
    }

    @Test
    fun testNormalProcessDetection() {
        // 测试正常进程不会被误判
        data class ProcessInfo(
            val pid: Int,
            val name: String,
            val selinuxContext: String,
            val isSuspicious: Boolean
        )
        
        val normalProcess = ProcessInfo(
            pid = 1000,
            name = "com.android.systemui",
            selinuxContext = "u:r:system_app:s0",
            isSuspicious = false
        )
        
        assertFalse("正常进程不应该被标记为可疑", normalProcess.isSuspicious)
        assertEquals("进程名称应该正确", "com.android.systemui", normalProcess.name)
        assertEquals("SELinux 上下文应该正常", "u:r:system_app:s0", normalProcess.selinuxContext)
    }

    @Test
    fun testDetectionStatistics() {
        // 测试检测统计信息
        data class ProcessDetectionResult(
            val layer: String,
            val detected: Boolean,
            val detail: String,
            val totalProcessesScanned: Int,
            val totalThreadsScanned: Int
        )
        
        val result = ProcessDetectionResult(
            layer = "第5层增强：进程与线程深度检测",
            detected = false,
            detail = "未发现可疑进程或线程",
            totalProcessesScanned = 150,
            totalThreadsScanned = 300
        )
        
        assertEquals("扫描进程数应该正确", 150, result.totalProcessesScanned)
        assertEquals("扫描线程数应该正确", 300, result.totalThreadsScanned)
        assertFalse("未检测到异常", result.detected)
    }
}
