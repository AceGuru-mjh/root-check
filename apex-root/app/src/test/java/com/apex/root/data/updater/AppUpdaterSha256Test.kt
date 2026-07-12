package com.apex.root.data.updater

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test
import java.lang.reflect.Method

/**
 * AppUpdater.extractExpectedSha256 单元测试 — P2-5
 *
 * 测试从 release notes 中提取 SHA-256 哈希的正则表达式,
 * 这是 APK 完整性校验的唯一防线,正则 bug 会让攻击者绕过验证。
 */
class AppUpdaterSha256Test {

    // 通过反射访问 internal 方法
    private fun extractExpectedSha256(releaseBody: String): String? {
        val method: Method = AppUpdater::class.java.getDeclaredMethod("extractExpectedSha256", String::class.java)
        method.isAccessible = true
        val constructor = AppUpdater::class.java.getDeclaredConstructor(
            android.content.Context::class.java
        )
        constructor.isAccessible = true
        val appUpdater = constructor.newInstance(org.mockito.Mockito.mock(android.content.Context::class.java))
        return method.invoke(appUpdater, releaseBody) as String?
    }

    @Test
    fun `extract SHA-256 with dash uppercase`() {
        val body = """
            ## v1.1.1 Release
            - Bug fixes
            SHA-256: a1b2c3d4e5f6789012345678901234567890abcdef1234567890abcdef123456
        """.trimIndent()
        val result = extractExpectedSha256(body)
        assertEquals("a1b2c3d4e5f6789012345678901234567890abcdef1234567890abcdef123456", result)
    }

    @Test
    fun `extract SHA-256 without dash lowercase`() {
        val body = "sha256: fedcba0987654321fedcba0987654321fedcba0987654321fedcba0987654321"
        val result = extractExpectedSha256(body)
        assertEquals("fedcba0987654321fedcba0987654321fedcba0987654321fedcba0987654321", result)
    }

    @Test
    fun `extract hash keyword`() {
        val body = "hash: 0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
        val result = extractExpectedSha256(body)
        assertEquals("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef", result)
    }

    @Test
    fun `extract with Chinese colon`() {
        val body = "SHA-256：abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789"
        val result = extractExpectedSha256(body)
        assertEquals("abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789", result)
    }

    @Test
    fun `return null when no hash present`() {
        val body = "## v1.1.1 Release\n- Bug fixes only\nNo hash here"
        val result = extractExpectedSha256(body)
        assertNull(result)
    }

    @Test
    fun `return null for empty body`() {
        val result = extractExpectedSha256("")
        assertNull(result)
    }

    @Test
    fun `return null for too-short hash`() {
        val body = "SHA-256: a1b2c3d4"
        val result = extractExpectedSha256(body)
        assertNull(result)
    }

    @Test
    fun `return null for non-hex characters`() {
        val body = "SHA-256: g1b2c3d4e5f6789012345678901234567890abcdef1234567890abcdef123456"
        val result = extractExpectedSha256(body)
        assertNull(result)
    }

    @Test
    fun `extract first hash when multiple present`() {
        val body = """
            SHA-256: 1111111111111111111111111111111111111111111111111111111111111111
            SHA-256: 2222222222222222222222222222222222222222222222222222222222222222
        """.trimIndent()
        val result = extractExpectedSha256(body)
        assertEquals("1111111111111111111111111111111111111111111111111111111111111111", result)
    }
}
