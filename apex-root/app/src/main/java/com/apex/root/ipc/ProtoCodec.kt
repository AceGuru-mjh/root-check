package com.apex.root.ipc

import java.io.ByteArrayOutputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder

/**
 * Protobuf wire format codec matching detection.proto schema.
 * Can be replaced with generated protobuf classes once the
 * protobuf Gradle plugin is enabled in build.gradle.kts.
 */
object ProtoCodec {

    // Wire types
    private const val VARINT = 0
    private const val FIXED32 = 5
    private const val FIXED64 = 1
    private const val LENGTH_DELIMITED = 2

    // Field numbers (matching detection.proto)
    private const val FIELD_REQUEST_ID = 1
    private const val FIELD_LEVEL = 2
    private const val FIELD_CONTEXT = 3
    private const val FIELD_PARAMETERS = 4
    private const val FIELD_NONCE = 5
    private const val FIELD_TIMESTAMP = 6

    private const val FIELD_SUCCESS = 2
    private const val FIELD_RESULT = 3
    private const val FIELD_CONFIDENCE = 4
    private const val FIELD_DESCRIPTION = 5
    private const val FIELD_SIGNATURE = 6

    private const val FIELD_SERVICE_ID = 1
    private const val FIELD_NAME = 2
    private const val FIELD_VERSION = 3
    private const val FIELD_WEIGHT = 4

    fun encodeVarint(value: Long): ByteArray {
        val out = ByteArrayOutputStream()
        var v = value
        while (v >= 0x80) {
            out.write(((v and 0x7F) or 0x80).toInt())
            v = v shr 7
        }
        out.write(v.toInt())
        return out.toByteArray()
    }

    fun decodeVarint(buf: ByteArray, offset: Int = 0): Pair<Long, Int> {
        var result = 0L
        var shift = 0
        var pos = offset
        while (pos < buf.size) {
            val b = buf[pos++].toInt() and 0xFF
            result = result or ((b and 0x7F).toLong() shl shift)
            if (b and 0x80 == 0) return Pair(result, pos - offset)
            shift += 7
        }
        return Pair(result, pos - offset)
    }

    fun encodeFixed32(value: Int): ByteArray {
        return ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(value).array()
    }

    fun encodeFixed64(value: Long): ByteArray {
        return ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN).putLong(value).array()
    }

    fun encodeTag(field: Int, wireType: Int): ByteArray = encodeVarint(((field shl 3) or wireType).toLong())

    fun encodeString(tag: ByteArray, value: String): ByteArray {
        val bytes = value.encodeToByteArray()
        return tag + encodeVarint(bytes.size.toLong()) + bytes
    }

    fun encodeBytes(tag: ByteArray, value: ByteArray): ByteArray {
        return tag + encodeVarint(value.size.toLong()) + value
    }

    fun encodeInt32(tag: ByteArray, value: Int): ByteArray = tag + encodeFixed32(value)

    fun encodeUInt64(tag: ByteArray, value: Long): ByteArray = tag + encodeVarint(value)

    fun encodeFloat(tag: ByteArray, value: Float): ByteArray {
        return tag + encodeFixed32(java.lang.Float.floatToIntBits(value))
    }

    fun encodeBool(tag: ByteArray, value: Boolean): ByteArray {
        return tag + encodeVarint(if (value) 1L else 0L)
    }

    fun encodeMapEntry(field: Int, key: String, value: ByteArray): ByteArray {
        val entry = ByteArrayOutputStream()
        val keyBytes = encodeString(
            encodeVarint(((1 shl 3) or LENGTH_DELIMITED).toLong()), key)
        val valBytes = encodeBytes(
            encodeVarint(((2 shl 3) or LENGTH_DELIMITED).toLong()), value)
        entry.write(keyBytes)
        entry.write(valBytes)
        val entryArray = entry.toByteArray()
        return encodeVarint(((field shl 3) or LENGTH_DELIMITED).toLong()) +
                encodeVarint(entryArray.size.toLong()) + entryArray
    }

    fun encodeDetectionRequest(
        requestId: String, level: Int, nonce: ByteArray,
        timestamp: Long, context: Map<String, ByteArray> = emptyMap(),
        parameters: Map<String, ByteArray> = emptyMap()
    ): ByteArray {
        val out = ByteArrayOutputStream()
        val tReq = encodeVarint(((FIELD_REQUEST_ID shl 3) or LENGTH_DELIMITED).toLong())
        out.write(encodeString(tReq, requestId))
        val tLvl = encodeVarint(((FIELD_LEVEL shl 3) or VARINT).toLong())
        out.write(encodeUInt64(tLvl, level.toLong()))
        val tNon = encodeVarint(((FIELD_NONCE shl 3) or LENGTH_DELIMITED).toLong())
        out.write(encodeBytes(tNon, nonce))
        val tTs = encodeVarint(((FIELD_TIMESTAMP shl 3) or VARINT).toLong())
        out.write(encodeUInt64(tTs, timestamp))
        for ((k, v) in context) {
            out.write(encodeMapEntry(FIELD_CONTEXT, k, v))
        }
        for ((k, v) in parameters) {
            out.write(encodeMapEntry(FIELD_PARAMETERS, k, v))
        }
        return out.toByteArray()
    }

    fun encodeDetectionResponse(
        requestId: String, success: Boolean, confidence: Float,
        description: String, signature: ByteArray,
        result: Map<String, ByteArray> = emptyMap()
    ): ByteArray {
        val out = ByteArrayOutputStream()
        val tReq = encodeVarint(((FIELD_REQUEST_ID shl 3) or LENGTH_DELIMITED).toLong())
        out.write(encodeString(tReq, requestId))
        val tSuc = encodeVarint(((FIELD_SUCCESS shl 3) or VARINT).toLong())
        out.write(encodeBool(tSuc, success))
        val tConf = encodeVarint(((FIELD_CONFIDENCE shl 3) or FIXED32).toLong())
        out.write(encodeFloat(tConf, confidence))
        val tDesc = encodeVarint(((FIELD_DESCRIPTION shl 3) or LENGTH_DELIMITED).toLong())
        out.write(encodeString(tDesc, description))
        val tSig = encodeVarint(((FIELD_SIGNATURE shl 3) or LENGTH_DELIMITED).toLong())
        out.write(encodeBytes(tSig, signature))
        for ((k, v) in result) {
            out.write(encodeMapEntry(FIELD_RESULT, k, v))
        }
        return out.toByteArray()
    }

    fun encodeServiceInfo(serviceId: String, name: String, version: String, weight: Int): ByteArray {
        val out = ByteArrayOutputStream()
        val tId = encodeVarint(((FIELD_SERVICE_ID shl 3) or LENGTH_DELIMITED).toLong())
        out.write(encodeString(tId, serviceId))
        val tName = encodeVarint(((FIELD_NAME shl 3) or LENGTH_DELIMITED).toLong())
        out.write(encodeString(tName, name))
        val tVer = encodeVarint(((FIELD_VERSION shl 3) or LENGTH_DELIMITED).toLong())
        out.write(encodeString(tVer, version))
        val tWt = encodeVarint(((FIELD_WEIGHT shl 3) or VARINT).toLong())
        out.write(encodeUInt64(tWt, weight.toLong()))
        return out.toByteArray()
    }
}
