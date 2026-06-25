package com.apex.root.domain.model

import com.apex.root.domain.guard.model.GuardState
import org.junit.Assert.*
import org.junit.Test

class GuardStateTest {

    @Test
    fun `GuardState default values`() {
        val state = GuardState(enabled = false, systemIntegrityOk = false, alertCount = 5)
        assertFalse(state.enabled)
        assertFalse(state.systemIntegrityOk)
        assertEquals(5, state.alertCount)
    }

    @Test
    fun `GuardState copy`() {
        val state = GuardState(enabled = true, systemIntegrityOk = true, alertCount = 0)
        val copy = state.copy(alertCount = 3)
        assertTrue(copy.enabled)
        assertTrue(copy.systemIntegrityOk)
        assertEquals(3, copy.alertCount)
    }
}
