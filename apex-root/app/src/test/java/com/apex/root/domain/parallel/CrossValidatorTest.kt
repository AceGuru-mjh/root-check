package com.apex.root.domain.parallel

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * CrossValidator 单元测试 — P2-6
 *
 * 测试交叉验证规则:
 *  - 7 条验证规则 (Magisk/KSU/APatch/SukiSU/Delta/Hook/Hide)
 *  - 2 条误报规则 (emulator/Compose multi-OAT)
 *  - 置信度计算
 *  - root 类型推断
 */
class CrossValidatorTest {

    private fun makeLayerResult(id: Int, detected: Boolean, weight: Int = 10): LayerResult {
        return LayerResult(
            layerId = id,
            layerName = "L$id",
            detected = detected,
            weight = weight,
            latencyMs = 100L
        )
    }

    private fun makeScanResult(detectedLayerIds: Set<Int>): ParallelScanResult {
        val layers = (1..20).map { id ->
            makeLayerResult(id, id in detectedLayerIds)
        }
        val detectedCount = layers.count { it.detected }
        val totalRisk = layers.filter { it.detected }.sumOf { it.riskContribution }.coerceAtMost(100)
        return ParallelScanResult(
            layers = layers,
            totalRiskScore = totalRisk,
            totalLatencyMs = 1000L,
            detectedCount = detectedCount,
            totalLayers = 19
        )
    }

    @Test
    fun `clean device returns zero confidence`() {
        val result = makeScanResult(emptySet())
        val validation = CrossValidator.validate(result)
        assertEquals(0, validation.confidence)
        assertEquals("UNKNOWN", validation.inferredRootType)
        assertFalse(validation.isHighConfidence)
    }

    @Test
    fun `Magisk confirmed by L8 plus L3`() {
        // L8 (Magisk daemon) + L3 (memory) = Magisk 多重信号确认
        val result = makeScanResult(setOf(8, 3))
        val validation = CrossValidator.validate(result)
        assertTrue("Should be high confidence", validation.isHighConfidence)
        assertEquals("MAGISK", validation.inferredRootType)
        assertTrue(validation.confidence >= 70)
    }

    @Test
    fun `Magisk confirmed by L8 plus L4`() {
        val result = makeScanResult(setOf(8, 4))
        val validation = CrossValidator.validate(result)
        assertTrue(validation.isHighConfidence)
        assertEquals("MAGISK", validation.inferredRootType)
    }

    @Test
    fun `KernelSU confirmed by L9 plus L6`() {
        val result = makeScanResult(setOf(9, 6))
        val validation = CrossValidator.validate(result)
        assertTrue(validation.isHighConfidence)
        assertEquals("KERNELSU", validation.inferredRootType)
    }

    @Test
    fun `APatch confirmed by L10 plus L18`() {
        val result = makeScanResult(setOf(10, 18))
        val validation = CrossValidator.validate(result)
        assertTrue(validation.isHighConfidence)
        assertEquals("APatch", validation.inferredRootType)
    }

    @Test
    fun `SukiSU confirmed by L17 plus L9`() {
        val result = makeScanResult(setOf(17, 9))
        val validation = CrossValidator.validate(result)
        assertTrue(validation.isHighConfidence)
        assertEquals("SUKISU", validation.inferredRootType)
    }

    @Test
    fun `Magisk Delta confirmed by L17 plus L8`() {
        val result = makeScanResult(setOf(17, 8))
        val validation = CrossValidator.validate(result)
        assertTrue(validation.isHighConfidence)
        assertEquals("MAGISK_DELTA", validation.inferredRootType)
    }

    @Test
    fun `single layer detection is low confidence`() {
        // Only L8 (Magisk daemon) without confirmation
        val result = makeScanResult(setOf(8))
        val validation = CrossValidator.validate(result)
        assertFalse("Single layer should not be high confidence", validation.isHighConfidence)
    }

    @Test
    fun `confidence is in valid range 0 to 100`() {
        // Test various combinations
        val testCases = listOf(
            emptySet<Int>(),
            setOf(1),
            setOf(1, 2, 3, 4, 5),
            setOf(8, 3, 4, 6, 16),
            (1..20).toSet()
        )
        for (detectedLayers in testCases) {
            val result = makeScanResult(detectedLayers)
            val validation = CrossValidator.validate(result)
            assertTrue(
                "Confidence ${validation.confidence} out of range for layers $detectedLayers",
                validation.confidence in 0..100
            )
        }
    }

    @Test
    fun `conclusion text is non-empty`() {
        val result = makeScanResult(setOf(8, 3))
        val validation = CrossValidator.validate(result)
        assertTrue("Conclusion should not be empty", validation.conclusion.isNotEmpty())
    }

    @Test
    fun `clean device conclusion contains safe`() {
        val result = makeScanResult(emptySet())
        val validation = CrossValidator.validate(result)
        assertTrue(validation.conclusion.contains("安全") || validation.conclusion.contains("SAFE"))
    }
}
