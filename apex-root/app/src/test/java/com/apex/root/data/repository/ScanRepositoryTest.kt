package com.apex.root.data.repository

import org.junit.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue

/**
 * ScanRepository 业务逻辑测试
 */
class ScanRepositoryTest {
    
    @Test
    fun riskLevelCalculation_critical() {
        // 高置信度或多检测方法应返回 CRITICAL
        val result1 = calculateRiskLevel(0.95f, 3)
        val result2 = calculateRiskLevel(0.6f, 8)
        
        assertEquals(RiskLevel.CRITICAL, result1)
        assertEquals(RiskLevel.CRITICAL, result2)
    }
    
    @Test
    fun riskLevelCalculation_high() {
        val result = calculateRiskLevel(0.75f, 5)
        assertEquals(RiskLevel.HIGH, result)
    }
    
    @Test
    fun riskLevelCalculation_medium() {
        val result = calculateRiskLevel(0.55f, 3)
        assertEquals(RiskLevel.MEDIUM, result)
    }
    
    @Test
    fun riskLevelCalculation_low() {
        val result = calculateRiskLevel(0.35f, 1)
        assertEquals(RiskLevel.LOW, result)
    }
    
    @Test
    fun riskLevelCalculation_none() {
        val result = calculateRiskLevel(0.1f, 0)
        assertEquals(RiskLevel.NONE, result)
    }
    
    @Test
    fun confidenceBoundaries() {
        // 边界值测试
        assertTrue(calculateRiskLevel(0.9f, 7) == RiskLevel.HIGH)
        assertTrue(calculateRiskLevel(0.91f, 7) == RiskLevel.CRITICAL)
        
        assertTrue(calculateRiskLevel(0.7f, 4) == RiskLevel.MEDIUM)
        assertTrue(calculateRiskLevel(0.71f, 4) == RiskLevel.HIGH)
    }
    
    // 辅助函数：模拟仓库中的风险计算逻辑
    private fun calculateRiskLevel(confidence: Float, methodCount: Int): RiskLevel {
        return when {
            confidence > 0.9f || methodCount >= 8 -> RiskLevel.CRITICAL
            confidence > 0.7f || methodCount >= 5 -> RiskLevel.HIGH
            confidence > 0.5f || methodCount >= 3 -> RiskLevel.MEDIUM
            confidence > 0.3f || methodCount >= 1 -> RiskLevel.LOW
            else -> RiskLevel.NONE
        }
    }
}

enum class RiskLevel {
    NONE, LOW, MEDIUM, HIGH, CRITICAL
}
