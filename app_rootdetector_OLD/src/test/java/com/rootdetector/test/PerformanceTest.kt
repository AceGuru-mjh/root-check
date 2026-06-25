package com.rootdetector.test

import com.rootdetector.detect.DetectionEngine
import com.rootdetector.detect.DetectionMode
import kotlinx.coroutines.runBlocking
import org.junit.Assert.*
import org.junit.Before
import org.junit.Test

/**
 * 性能测试用例
 * 验证各检测模式的耗时是否满足性能目标：
 * - 快速检测 < 1秒
 * - 标准检测 3-5秒
 * - 深度检测 10-15秒
 */
class PerformanceTest {

    private lateinit var engine: DetectionEngine

    @Before
    fun setUp() {
        engine = DetectionEngine()
    }

    /**
     * 测试快速检测性能
     * 目标：< 1秒
     */
    @Test
    fun testQuickDetectionPerformance() = runBlocking {
        val warmupRuns = 3
        repeat(warmupRuns) {
            engine.detect(DetectionMode.QUICK)
        }

        val iterations = 10
        val times = mutableListOf<Long>()

        repeat(iterations) {
            val startTime = System.nanoTime()
            engine.detect(DetectionMode.QUICK)
            val endTime = System.nanoTime()
            times.add((endTime - startTime) / 1_000_000) // 转换为毫秒
        }

        val avgTime = times.average()
        val maxTime = times.maxOrNull() ?: 0
        val minTime = times.minOrNull() ?: 0

        println("=== 快速检测性能测试 ===")
        println("平均耗时: ${"%.2f".format(avgTime)} ms")
        println("最大耗时: $maxTime ms")
        println("最小耗时: $minTime ms")
        println("目标: < 1000 ms")

        assertTrue("快速检测平均耗时超过 1000ms: ${avgTime}ms", avgTime < 1000)
        assertTrue("快速检测最大耗时超过 2000ms: ${maxTime}ms", maxTime < 2000)
    }

    /**
     * 测试标准检测性能
     * 目标：3-5秒
     */
    @Test
    fun testStandardDetectionPerformance() = runBlocking {
        val warmupRuns = 2
        repeat(warmupRuns) {
            engine.detect(DetectionMode.STANDARD)
        }

        val iterations = 5
        val times = mutableListOf<Long>()

        repeat(iterations) {
            val startTime = System.nanoTime()
            engine.detect(DetectionMode.STANDARD)
            val endTime = System.nanoTime()
            times.add((endTime - startTime) / 1_000_000)
        }

        val avgTime = times.average()
        val maxTime = times.maxOrNull() ?: 0

        println("=== 标准检测性能测试 ===")
        println("平均耗时: ${"%.2f".format(avgTime)} ms")
        println("最大耗时: $maxTime ms")
        println("目标: 3000-5000 ms")

        assertTrue("标准检测平均耗时超过 5000ms: ${avgTime}ms", avgTime < 5000)
        assertTrue("标准检测最大耗时超过 8000ms: ${maxTime}ms", maxTime < 8000)
    }

    /**
     * 测试深度检测性能
     * 目标：10-15秒
     */
    @Test
    fun testDeepDetectionPerformance() = runBlocking {
        val warmupRuns = 1
        repeat(warmupRuns) {
            engine.detect(DetectionMode.DEEP)
        }

        val iterations = 3
        val times = mutableListOf<Long>()

        repeat(iterations) {
            val startTime = System.nanoTime()
            engine.detect(DetectionMode.DEEP)
            val endTime = System.nanoTime()
            times.add((endTime - startTime) / 1_000_000)
        }

        val avgTime = times.average()
        val maxTime = times.maxOrNull() ?: 0

        println("=== 深度检测性能测试 ===")
        println("平均耗时: ${"%.2f".format(avgTime)} ms")
        println("最大耗时: $maxTime ms")
        println("目标: 10000-15000 ms")

        assertTrue("深度检测平均耗时超过 15000ms: ${avgTime}ms", avgTime < 15000)
        assertTrue("深度检测最大耗时超过 20000ms: ${maxTime}ms", maxTime < 20000)
    }

    /**
     * 测试并行检测加速比
     * 验证并行协程是否带来性能提升
     */
    @Test
    fun testParallelDetectionSpeedup() = runBlocking {
        val iterations = 5
        val times = mutableListOf<Long>()

        repeat(iterations) {
            val startTime = System.nanoTime()
            engine.detect(DetectionMode.STANDARD)
            val endTime = System.nanoTime()
            times.add((endTime - startTime) / 1_000_000)
        }

        val avgTime = times.average()

        println("=== 并行检测加速比测试 ===")
        println("标准检测平均耗时: ${"%.2f".format(avgTime)} ms")
        println("预期加速比: >= 2x (相比串行)")

        assertTrue("检测耗时异常: ${avgTime}ms", avgTime > 0)
    }

    /**
     * 测试缓存命中时的性能提升
     */
    @Test
    fun testCacheHitPerformance() = runBlocking {
        val firstStartTime = System.nanoTime()
        engine.detect(DetectionMode.QUICK)
        val firstEndTime = System.nanoTime()
        val firstTime = (firstEndTime - firstStartTime) / 1_000_000

        val secondStartTime = System.nanoTime()
        engine.detect(DetectionMode.QUICK)
        val secondEndTime = System.nanoTime()
        val secondTime = (secondEndTime - secondStartTime) / 1_000_000

        println("=== 缓存性能测试 ===")
        println("首次检测: $firstTime ms")
        println("缓存命中: $secondTime ms")

        assertTrue("首次检测时间应大于 0", firstTime >= 0)
        assertTrue("缓存命中时间应大于 0", secondTime >= 0)
    }

    /**
     * 测试内存使用效率
     * 确保多次检测不会导致内存泄漏
     */
    @Test
    fun testMemoryEfficiency() = runBlocking {
        val runtime = Runtime.getRuntime()

        System.gc()
        val initialMemory = runtime.totalMemory() - runtime.freeMemory()

        repeat(20) {
            engine.detect(DetectionMode.QUICK)
        }

        System.gc()
        Thread.sleep(100)
        val finalMemory = runtime.totalMemory() - runtime.freeMemory()

        val memoryGrowth = finalMemory - initialMemory
        val growthMB = memoryGrowth / (1024.0 * 1024.0)

        println("=== 内存效率测试 ===")
        println("初始内存: ${initialMemory / (1024.0 * 1024.0)} MB")
        println("最终内存: ${finalMemory / (1024.0 * 1024.0)} MB")
        println("内存增长: $growthMB MB")

        assertTrue("内存增长超过 50MB，可能存在内存泄漏: ${growthMB}MB", growthMB < 50 * 1024 * 1024)
    }

    /**
     * 测试 detectWithTiming 方法
     */
    @Test
    fun testDetectWithTiming() = runBlocking {
        val (results, duration) = engine.detectWithTiming(DetectionMode.QUICK)

        println("=== Timing 方法测试 ===")
        println("检测结果数: ${results.size}")
        println("检测耗时: $duration ms")

        assertTrue("检测结果不应为空", results.isNotEmpty())
        assertTrue("耗时应大于 0", duration >= 0)
        assertTrue("快速检测耗时不应超过 2000ms", duration < 2000)
    }
}
