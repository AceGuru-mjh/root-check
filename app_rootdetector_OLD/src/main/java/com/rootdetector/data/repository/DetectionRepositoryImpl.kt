package com.rootdetector.data.repository

import com.rootdetector.data.datasource.NativeDataSource
import com.rootdetector.domain.model.DetectionLevel
import com.rootdetector.domain.model.DetectionResult
import com.rootdetector.domain.model.PluginResult
import com.rootdetector.domain.model.ThreatLevel
import com.rootdetector.domain.repository.DetectionRepository
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.json.JSONObject

class DetectionRepositoryImpl(
    private val nativeDataSource: NativeDataSource
) : DetectionRepository {

    override suspend fun detect(level: DetectionLevel): DetectionResult = withContext(Dispatchers.IO) {
        val layerMask = when (level) {
            DetectionLevel.QUICK    -> 0x0A
            DetectionLevel.STANDARD -> 0x3E
            DetectionLevel.DEEP     -> 0x7F
            DetectionLevel.FORENSIC -> 0xFF
        }
        val json = nativeDataSource.runDetection(layerMask)
        parseJsonResult(json)
    }

    override suspend fun quickDetect(): DetectionResult = withContext(Dispatchers.IO) {
        val json = nativeDataSource.quickDetect()
        parseJsonResult(json)
    }

    override fun isDaemonRunning(): Boolean {
        val result = nativeDataSource.quickDetect()
        return result != null && !result.contains("daemon_not_running")
    }

    override suspend fun startDaemon(): Boolean = withContext(Dispatchers.IO) {
        nativeDataSource.startDaemon()
    }

    private fun parseJsonResult(json: String?): DetectionResult {
        if (json == null) {
            return DetectionResult(
                layerResults = emptyList(),
                rootDetected = false,
                riskScore = 0,
                reportId = "error_null"
            )
        }
        return try {
            val obj = JSONObject(json)
            val layerId = obj.optInt("layer_id", 0)
            val detected = obj.optBoolean("detected", false)
            val confidence = obj.optInt("confidence", 0)
            val riskScore = obj.optInt("risk_score", 0)
            val costNs = obj.optLong("cost_ns", 0)
            val detail = obj.optString("detail", "")
            val error = obj.optString("error", "")

            if (error.isNotEmpty()) {
                DetectionResult(
                    layerResults = emptyList(),
                    rootDetected = false,
                    riskScore = 0,
                    reportId = "error_$error"
                )
            } else {
                val result = PluginResult(
                    layerId = layerId,
                    detected = detected,
                    confidence = confidence,
                    riskScore = riskScore,
                    costNs = costNs,
                    detail = detail
                )
                DetectionResult(
                    layerResults = listOf(result),
                    rootDetected = detected,
                    riskScore = riskScore,
                    reportId = "report_${System.currentTimeMillis()}"
                )
            }
        } catch (e: Exception) {
            DetectionResult(
                layerResults = emptyList(),
                rootDetected = false,
                riskScore = 0,
                reportId = "error_parse"
            )
        }
    }
}