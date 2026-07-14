package com.apex.root.data

import org.junit.Assert.*
import org.junit.Test

class FixRecommendationsTest {

    @Test
    fun `getRecommendation for known layer`() {
        val rec = FixRecommendations.getRecommendation("属性")
        assertNotNull(rec)
        assertEquals("properties", rec?.id)
        assertTrue(rec?.stepsZh?.isNotEmpty() == true)
    }

    @Test
    fun `getRecommendation returns null for unknown layer`() {
        assertNull(FixRecommendations.getRecommendation("unknown_layer_xyz"))
    }

    @Test
    fun `getRecommendation matches specific before generic`() {
        val generic = FixRecommendations.getRecommendation("固件")
        val specific = FixRecommendations.getRecommendation("固件完整性")
        assertNotNull(generic)
        assertNotNull(specific)
    }

    @Test
    fun `getAllRecommendations returns all entries`() {
        val all = FixRecommendations.getAllRecommendations()
        assertTrue(all.size >= 10)
    }

    @Test
    fun `getRecommendationsForLayers returns sorted by priority`() {
        val recs = FixRecommendations.getRecommendationsForLayers(listOf("内核", "属性"))
        assertTrue(recs.isNotEmpty())
        for (i in 1 until recs.size) {
            assertTrue(recs[i - 1].priority >= recs[i].priority)
        }
    }

    @Test
    fun `getRecommendationsForLayers deduplicates by id`() {
        val recs = FixRecommendations.getRecommendationsForLayers(listOf("内核", "内核模块"))
        val ids = recs.map { it.id }
        assertEquals(ids.toSet().size, ids.size)
    }

    @Test
    fun `recommendation has required fields`() {
        val rec = FixRecommendations.getRecommendation("APatch")!!
        assertTrue(rec.titleZh.isNotBlank())
        assertTrue(rec.titleEn.isNotBlank())
        assertTrue(rec.descriptionZh.isNotBlank())
        assertTrue(rec.descriptionEn.isNotBlank())
        assertTrue(rec.stepsZh.isNotEmpty())
        assertTrue(rec.stepsEn.isNotEmpty())
        assertTrue(rec.priority in 1..10)
    }

    @Test
    fun `priority 10 items are most critical`() {
        val apatch = FixRecommendations.getRecommendation("APatch")!!
        val kernel = FixRecommendations.getRecommendation("内核")!!
        assertEquals(10, apatch.priority)
        assertEquals(10, kernel.priority)
    }
}
