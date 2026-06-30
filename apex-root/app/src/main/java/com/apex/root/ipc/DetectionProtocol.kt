package com.apex.root.ipc

import com.apex.root.domain.trust.model.*
import java.io.ByteArrayInputStream
import java.io.ByteArrayOutputStream
import java.io.DataInputStream
import java.io.DataOutputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.UUID

object DetectionProtocol {

    const val MAGIC_REPORT: Byte = 0x01
    const val MAGIC_ALERT: Byte = 0x02
    const val MAGIC_PROGRESS: Byte = 0x03
    const val MAGIC_HEARTBEAT: Byte = 0x04

    private const val PROTOCOL_VERSION: Byte = 0x01

    fun encodeReport(report: GlobalSecureReport): ByteArray {
        val baos = ByteArrayOutputStream()
        val dos = DataOutputStream(baos)

        dos.writeByte(MAGIC_REPORT.toInt())
        dos.writeByte(PROTOCOL_VERSION.toInt())
        dos.writeLong(report.timestamp)
        writeString(dos, report.taskId, 36)
        writeString(dos, report.reportId, 36)
        dos.writeFloat(report.riskScore)
        dos.writeByte(report.overallRisk.ordinal)
        dos.writeInt(report.results.size)

        for (result in report.results) {
            writeString(dos, result.serviceId, 32)
            writeString(dos, result.serviceName, 64)
            dos.writeBoolean(result.success)
            dos.writeFloat(result.confidence)
            dos.writeLong(result.durationMs)
            dos.writeShort(result.findings.size)
            for (finding in result.findings) {
                dos.writeByte(finding.type.ordinal)
                dos.writeByte(finding.severity.ordinal)
                writeString(dos, finding.description, 128)
                writeString(dos, finding.evidence, 256)
            }
            writeBytes(dos, result.rawHash)
            dos.writeShort(result.signatures.size)
            for (sig in result.signatures) writeBytes(dos, sig)
        }

        dos.writeInt(report.consensusSignatures.size)
        for (sig in report.consensusSignatures) writeBytes(dos, sig)
        writeBytes(dos, report.daemonSignature)

        return baos.toByteArray()
    }

    fun decodeReport(data: ByteArray): GlobalSecureReport? {
        try {
            val dis = DataInputStream(ByteArrayInputStream(data))
            val magic = dis.readByte()
            if (magic.toInt() != MAGIC_REPORT.toInt()) return null
            val version = dis.readByte()
            val timestamp = dis.readLong()
            val taskId = readString(dis, 36)
            val reportId = readString(dis, 36)
            val riskScore = dis.readFloat()
            val overallRiskOrd = dis.readByte()
            val overallRisk = Severity.entries.getOrElse(overallRiskOrd.toInt()) { Severity.SAFE }
            val layerCount = dis.readInt()

            val results = mutableListOf<TrustedLayerResult>()
            for (i in 0 until layerCount) {
                val serviceId = readString(dis, 32)
                val serviceName = readString(dis, 64)
                val success = dis.readBoolean()
                val confidence = dis.readFloat()
                val durationMs = dis.readLong()
                val findingCount = dis.readUnsignedShort()
                val findings = mutableListOf<Finding>()
                for (j in 0 until findingCount) {
                    val ftOrd = dis.readByte().toInt()
                    val sevOrd = dis.readByte().toInt()
                    val desc = readString(dis, 128)
                    val evidence = readString(dis, 256)
                    findings.add(Finding(
                        type = FindingType.entries.getOrElse(ftOrd) { FindingType.ROOT_BINARY },
                        severity = Severity.entries.getOrElse(sevOrd) { Severity.INFO },
                        description = desc,
                        evidence = evidence
                    ))
                }
                val rawHash = readBytes(dis, 64)
                val sigCount = dis.readUnsignedShort()
                val signatures = mutableListOf<ByteArray>()
                for (j in 0 until sigCount) signatures.add(readBytes(dis))
                results.add(TrustedLayerResult(
                    serviceId = serviceId,
                    serviceName = serviceName,
                    success = success,
                    confidence = confidence,
                    findings = findings,
                    rawHash = rawHash,
                    signatures = signatures,
                    durationMs = durationMs
                ))
            }

            val consensusCount = dis.readInt()
            val consensusSigs = mutableListOf<ByteArray>()
            for (i in 0 until consensusCount) consensusSigs.add(readBytes(dis))
            val daemonSig = readBytes(dis)

            return GlobalSecureReport(
                reportId = reportId,
                timestamp = timestamp,
                taskId = taskId,
                results = results,
                consensusSignatures = consensusSigs,
                overallRisk = overallRisk,
                riskScore = riskScore,
                daemonSignature = daemonSig
            )
        } catch (_: Exception) {
            return null
        }
    }

    fun encodeAlert(alert: SecurityAlert): ByteArray {
        val baos = ByteArrayOutputStream()
        val dos = DataOutputStream(baos)
        dos.writeByte(MAGIC_ALERT.toInt())
        dos.writeByte(PROTOCOL_VERSION.toInt())
        writeString(dos, alert.alertId, 36)
        dos.writeByte(alert.type.ordinal)
        dos.writeByte(alert.severity.ordinal)
        writeString(dos, alert.description, 256)
        writeString(dos, alert.sourceReplica, 16)
        dos.writeLong(alert.timestamp)
        writeBytes(dos, alert.evidence)
        return baos.toByteArray()
    }

    fun decodeAlert(data: ByteArray): SecurityAlert? {
        try {
            val dis = DataInputStream(ByteArrayInputStream(data))
            val magic = dis.readByte()
            if (magic.toInt() != MAGIC_ALERT.toInt()) return null
            dis.readByte() // version
            val alertId = readString(dis, 36)
            val typeOrd = dis.readByte().toInt()
            val sevOrd = dis.readByte().toInt()
            val desc = readString(dis, 256)
            val source = readString(dis, 16)
            val timestamp = dis.readLong()
            val evidence = readBytes(dis)
            return SecurityAlert(
                alertId = alertId,
                type = AlertType.entries.getOrElse(typeOrd) { AlertType.PROCESS_TAMPER },
                severity = Severity.entries.getOrElse(sevOrd) { Severity.CRITICAL },
                description = desc,
                sourceReplica = source,
                timestamp = timestamp,
                evidence = evidence
            )
        } catch (_: Exception) {
            return null
        }
    }

    fun encodeProgress(progress: Float): ByteArray {
        return byteArrayOf(MAGIC_PROGRESS, (progress.coerceIn(0f, 1f) * 100).toInt().toByte())
    }

    fun encodeHeartbeat(replicaId: Int): ByteArray {
        val buf = ByteBuffer.allocate(10).order(ByteOrder.BIG_ENDIAN)
        buf.put(MAGIC_HEARTBEAT)
        buf.put(PROTOCOL_VERSION)
        buf.put(replicaId.toByte())
        buf.putLong(System.nanoTime())
        return buf.array()
    }

    private fun writeString(dos: DataOutputStream, s: String, maxLen: Int) {
        val bytes = s.encodeToByteArray()
        dos.writeInt(minOf(bytes.size, maxLen))
        dos.write(bytes, 0, minOf(bytes.size, maxLen))
    }

    private fun readString(dis: DataInputStream, maxLen: Int): String {
        val len = minOf(dis.readInt(), maxLen)
        val bytes = ByteArray(len)
        dis.readFully(bytes)
        return bytes.decodeToString()
    }

    private fun writeBytes(dos: DataOutputStream, data: ByteArray) {
        dos.writeInt(data.size)
        dos.write(data)
    }

    private fun readBytes(dis: DataInputStream, fixedLen: Int = -1): ByteArray {
        val len = if (fixedLen >= 0) fixedLen else dis.readInt()
        val data = ByteArray(len)
        dis.readFully(data)
        return data
    }
}
