package com.rootdetector.test

import com.rootdetector.detector.SyscallTimingDetector
import com.rootdetector.detect.DetectionResult
import kotlinx.coroutines.runBlocking
import org.junit.Test
import org.junit.Assert.*

/**
 * 系统调用时间检测单元测试
 */
class SyscallTimingDetectorTest {

    @Test
    fun testSyscallTimingDetection() = runBlocking {
        val detector = SyscallTimingDetector()
        val result: DetectionResult = detector.detect()
        
        // 验证结果不为空
        assertNotNull("检测结果不应为空", result)
        
        // 验证层名称正确
        assertEquals("检测层名称应正确", "第5层：系统调用时间分析", result.layer)
        
        // 验证详细信息不为空
        assertTrue("详细信息不应为空", result.detail.isNotEmpty())
        
        // 打印结果供调试
        println("检测结果: detected=${result.detected}")
        println("检测层: ${result.layer}")
        println("详细信息: ${result.detail}")
    }
    
    @Test
    fun testSyscallTimingConsistency() = runBlocking {
        val detector = SyscallTimingDetector()
        
        // 多次检测，验证结果一致性
        val results = (1..3).map {
            detector.detect()
        }
        
        // 所有检测的 detected 状态应该一致
        val firstDetected = results[0].detected
        assertTrue("多次检测结果应一致", 
            results.all { it.detected == firstDetected })
    }
    
    @Test
    fun testSyscallTimingPerformance() = runBlocking {
        val detector = SyscallTimingDetector()
        
        // 测量检测耗时
        val startTime = System.currentTimeMillis()
        val result = detector.detect()
        val duration = System.currentTimeMillis() - startTime
        
        // 验证检测耗时在合理范围内（应该小于 10 秒）
        assertTrue("检测耗时应合理（< 10秒）", duration < 10000)
        
        println("检测耗时: ${duration}ms")
        println("检测结果: ${result.detail}")
    }
}
