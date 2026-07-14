package com.apex.root.ipc

import com.apex.root.domain.trust.model.*
import org.junit.Assert.*
import org.junit.Test

class DetectionProtocolTest {

    @Test
    fun `encode and decode report roundtrip`() {
        val report = GlobalSecureReport(
            reportId = "rep-123",
            timestamp = 1000L,
            taskId = "task-456",
            results = listOf(
                TrustedLayerResult(
                    serviceId = "srv1",
                    serviceName = "Memory Scan",
                    success = true,
                    confidence = 0.95f,
                    findings = listOf(
                        Finding(
                            type = FindingType.ROOT_BINARY,
                            severity = Severity.HIGH,
                            description = "Found su binary",
                            evidence = "/system/xbin/su"
                        )
                    ),
                    rawHash = ByteArray(64) { 1 },
                    signatures = listOf(ByteArray(64) { 2 }),
                    durationMs = 150L
                )
            ),
            consensusSignatures = listOf(ByteArray(64) { 3 }),
            overallRisk = Severity.HIGH,
            riskScore = 75.5f,
            daemonSignature = ByteArray(64) { 4 }
        )

        val encoded = DetectionProtocol.encodeReport(report)
        assertTrue(encoded.isNotEmpty())

        val decoded = DetectionProtocol.decodeReport(encoded)
        assertNotNull(decoded)
        assertEquals(report.reportId, decoded!!.reportId)
        assertEquals(report.taskId, decoded.taskId)
        assertEquals(report.timestamp, decoded.timestamp)
        assertEquals(report.overallRisk, decoded.overallRisk)
        assertEquals(report.riskScore, decoded.riskScore, 0.01f)
        assertEquals(report.results.size, decoded.results.size)
        assertEquals(report.results[0].serviceId, decoded.results[0].serviceId)
        assertEquals(report.results[0].findings.size, decoded.results[0].findings.size)
        assertEquals(
            report.results[0].findings[0].type,
            decoded.results[0].findings[0].type
        )
    }

    @Test
    fun `decode report with invalid magic returns null`() {
        val invalid = byteArrayOf(0xFF.toByte(), 0x01)
        assertNull(DetectionProtocol.decodeReport(invalid))
    }

    @Test
    fun `decode report with truncated data returns null`() {
        val invalid = byteArrayOf(0x01, 0x01)
        assertNull(DetectionProtocol.decodeReport(invalid))
    }

    @Test
    fun `encode and decode alert roundtrip`() {
        val alert = SecurityAlert(
            alertId = "alert-001",
            type = AlertType.PROCESS_TAMPER,
            severity = Severity.CRITICAL,
            description = "Process modified",
            sourceReplica = "replica-1",
            timestamp = 2000L,
            evidence = ByteArray(32) { 0xAB }
        )

        val encoded = DetectionProtocol.encodeAlert(alert)
        assertTrue(encoded.isNotEmpty())

        val decoded = DetectionProtocol.decodeAlert(encoded)
        assertNotNull(decoded)
        assertEquals(alert.alertId, decoded!!.alertId)
        assertEquals(alert.type, decoded.type)
        assertEquals(alert.severity, decoded.severity)
        assertEquals(alert.description, decoded.description)
        assertEquals(alert.sourceReplica, decoded.sourceReplica)
        assertEquals(alert.timestamp, decoded.timestamp)
    }

    @Test
    fun `decode alert with invalid magic returns null`() {
        val invalid = byteArrayOf(0xFF.toByte(), 0x01)
        assertNull(DetectionProtocol.decodeAlert(invalid))
    }

    @Test
    fun `encode progress clamps to valid range`() {
        val below = DetectionProtocol.encodeProgress(-0.5f)
        assertEquals(0, below[1].toInt() and 0xFF)

        val above = DetectionProtocol.encodeProgress(1.5f)
        assertEquals(100, above[1].toInt() and 0xFF)

        val normal = DetectionProtocol.encodeProgress(0.5f)
        assertEquals(50, normal[1].toInt() and 0xFF)
    }

    @Test
    fun `encode heartbeat contains magic and version`() {
        val hb = DetectionProtocol.encodeHeartbeat(1)
        assertEquals(DetectionProtocol.MAGIC_HEARTBEAT, hb[0])
        assertEquals(0x01, hb[1].toInt() and 0xFF)
    }
}
