package com.apex.root.data

import org.junit.After
import org.junit.Assert.*
import org.junit.Test

class DetectionCacheTest {

    @After
    fun tearDown() {
        DetectionCache.invalidateAll()
    }

    @Test
    fun `putString and getString roundtrip`() {
        DetectionCache.putString("test_key", "hello")
        assertEquals("hello", DetectionCache.getString("test_key"))
    }

    @Test
    fun `putInt and getInt roundtrip`() {
        DetectionCache.putInt("score", 42)
        assertEquals(42, DetectionCache.getInt("score"))
    }

    @Test
    fun `putBoolean and getBoolean roundtrip`() {
        DetectionCache.putBoolean("flag", true)
        assertEquals(true, DetectionCache.getBoolean("flag"))
        DetectionCache.putBoolean("flag", false)
        assertEquals(false, DetectionCache.getBoolean("flag"))
    }

    @Test
    fun `get returns null for missing key`() {
        assertNull(DetectionCache.getString("nonexistent"))
        assertNull(DetectionCache.getInt("nonexistent"))
        assertNull(DetectionCache.getBoolean("nonexistent"))
    }

    @Test
    fun `invalidate removes single key`() {
        DetectionCache.putString("temp", "value")
        assertNotNull(DetectionCache.getString("temp"))
        DetectionCache.invalidate("temp")
        assertNull(DetectionCache.getString("temp"))
    }

    @Test
    fun `invalidateAll clears all entries`() {
        DetectionCache.putString("a", "1")
        DetectionCache.putString("b", "2")
        DetectionCache.invalidateAll()
        assertNull(DetectionCache.getString("a"))
        assertNull(DetectionCache.getString("b"))
    }

    @Test
    fun `cache respects TTL`() {
        DetectionCache.putString("ttl_test", "fresh")
        assertEquals("fresh", DetectionCache.getString("ttl_test"))
    }

    @Test
    fun `stats update on hit`() {
        DetectionCache.putString("stats_key", "val")
        DetectionCache.getString("stats_key")
        val stats = DetectionCache.stats.value
        assertTrue(stats.hitCount > 0)
    }

    @Test
    fun `stats update on miss`() {
        DetectionCache.getString("miss_key")
        val stats = DetectionCache.stats.value
        assertEquals(1, stats.missCount)
    }

    @Test
    fun `known keys are defined`() {
        assertEquals("quick_scan", DetectionCache.KEY_QUICK_SCAN)
        assertEquals("risk_score", DetectionCache.KEY_RISK_SCORE)
        assertEquals("is_rooted", DetectionCache.KEY_IS_ROOTED)
        assertEquals("mem_fingerprint", DetectionCache.KEY_MEM_FINGERPRINT)
        assertEquals("shamiko", DetectionCache.KEY_SHAMIKO)
        assertEquals("zygisk_next", DetectionCache.KEY_ZYGISK_NEXT)
    }
}
