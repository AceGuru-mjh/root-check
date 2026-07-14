package com.apex.root.ipc

import org.junit.Assert.*
import org.junit.Test

class ProtoCodecTest {

    @Test
    fun `encode and decode varint`() {
        val testValues = listOf(0L, 1L, 127L, 128L, 255L, 256L, 65535L, 100000L, Long.MAX_VALUE)

        for (v in testValues) {
            val encoded = ProtoCodec.encodeVarint(v)
            val (decoded, bytesRead) = ProtoCodec.decodeVarint(encoded)
            assertEquals("Failed for value $v", v, decoded)
            assertTrue("Invalid bytesRead for value $v", bytesRead > 0)
        }
    }

    @Test
    fun `encode fixed32`() {
        val encoded = ProtoCodec.encodeFixed32(0x12345678)
        assertEquals(4, encoded.size)
        assertEquals(0x78, encoded[0].toInt() and 0xFF)
        assertEquals(0x56, encoded[1].toInt() and 0xFF)
        assertEquals(0x34, encoded[2].toInt() and 0xFF)
        assertEquals(0x12, encoded[3].toInt() and 0xFF)
    }

    @Test
    fun `encode fixed64`() {
        val encoded = ProtoCodec.encodeFixed64(0x0123456789ABCDEFL)
        assertEquals(8, encoded.size)
    }

    @Test
    fun `encodeTag produces correct wire format`() {
        val tag5varint = ProtoCodec.encodeTag(5, 0)
        val (decoded, _) = ProtoCodec.decodeVarint(tag5varint)
        assertEquals((5 shl 3) or 0, decoded.toInt())
    }

    @Test
    fun `encodeString includes length prefix and content`() {
        val tag = ProtoCodec.encodeTag(1, 2)
        val result = ProtoCodec.encodeString(tag, "hello")
        assertTrue(result.size > 5)
    }

    @Test
    fun `encodeBool produces correct values`() {
        val tag = ProtoCodec.encodeTag(1, 0)
        val trueEnc = ProtoCodec.encodeBool(tag, true)
        val falseEnc = ProtoCodec.encodeBool(tag, false)

        assertTrue(trueEnc.size > 1)
        assertTrue(falseEnc.size > 1)

        val (trueVal, _) = ProtoCodec.decodeVarint(trueEnc.drop(tag.size).toByteArray())
        val (falseVal, _) = ProtoCodec.decodeVarint(falseEnc.drop(tag.size).toByteArray())
        assertEquals(1L, trueVal)
        assertEquals(0L, falseVal)
    }

    @Test
    fun `encodeDetectionRequest produces non-empty result`() {
        val result = ProtoCodec.encodeDetectionRequest(
            requestId = "req-1",
            level = 2,
            nonce = ByteArray(32) { 0x42 },
            timestamp = 1000L
        )
        assertTrue(result.isNotEmpty())
    }

    @Test
    fun `encodeDetectionRequest with context and parameters`() {
        val result = ProtoCodec.encodeDetectionRequest(
            requestId = "req-2",
            level = 3,
            nonce = ByteArray(32),
            timestamp = 2000L,
            context = mapOf("device" to "pixel".encodeToByteArray()),
            parameters = mapOf("depth" to "full".encodeToByteArray())
        )
        assertTrue(result.isNotEmpty())
    }

    @Test
    fun `encodeDetectionResponse produces non-empty result`() {
        val result = ProtoCodec.encodeDetectionResponse(
            requestId = "req-1",
            success = true,
            confidence = 0.95f,
            description = "All clear",
            signature = ByteArray(64) { 0xAA }
        )
        assertTrue(result.isNotEmpty())
    }

    @Test
    fun `encodeServiceInfo produces non-empty result`() {
        val result = ProtoCodec.encodeServiceInfo(
            serviceId = "ms001",
            name = "File Scanner",
            version = "1.0",
            weight = 5
        )
        assertTrue(result.isNotEmpty())
    }

    @Test
    fun `encodeVarint for edge values`() {
        val encoded = ProtoCodec.encodeVarint(0)
        assertEquals(1, encoded.size)
        assertEquals(0, encoded[0].toInt())
    }
}
