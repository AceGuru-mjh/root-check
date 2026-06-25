package com.apex.root.domain.model

import org.junit.Assert.*
import org.junit.Test

class ScanResultTest {

    @Test
    fun `RootType fromValue returns correct enum`() {
        assertEquals(RootType.NONE, RootType.fromValue(0))
        assertEquals(RootType.MAGISK, RootType.fromValue(1))
        assertEquals(RootType.KERNELSU, RootType.fromValue(2))
        assertEquals(RootType.APATCH, RootType.fromValue(3))
        assertEquals(RootType.UNKNOWN, RootType.fromValue(4))
        assertEquals(RootType.UNKNOWN, RootType.fromValue(99))
    }

    @Test
    fun `RiskLevel ordering is correct`() {
        val levels = RiskLevel.entries
        assertEquals(4, levels.size)
        assertEquals(RiskLevel.SAFE, levels[0])
        assertEquals(RiskLevel.WARNING, levels[1])
        assertEquals(RiskLevel.DANGER, levels[2])
        assertEquals(RiskLevel.CRITICAL, levels[3])
    }

    @Test
    fun `CureLevel labels are correct`() {
        assertEquals("轻度清理", CureLevel.LIGHT.label)
        assertEquals("标准修复", CureLevel.STANDARD.label)
        assertEquals("深度恢复", CureLevel.DEEP.label)
        assertEquals("完全重置", CureLevel.FACTORY.label)
    }

    @Test
    fun `ScanResult with defaults uses current timestamp`() {
        val before = System.currentTimeMillis()
        val result = ScanResult(isRooted = false, riskLevel = RiskLevel.SAFE, details = "OK")
        val after = System.currentTimeMillis()
        assertTrue(result.timestamp in before..after)
        assertEquals(0, result.riskScore)
    }

    @Test
    fun `GameModeState default values`() {
        val state = GameModeState()
        assertFalse(state.active)
        assertEquals(0, state.hiddenProcesses)
    }
}
