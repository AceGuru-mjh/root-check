package com.rootdetector.detect

import com.rootdetector.detector.BinderLatencyDetector
import com.rootdetector.detector.CacheTimingDetector
import com.rootdetector.detector.InterruptTimingDetector
import com.rootdetector.detector.PagefaultMonitor
import com.rootdetector.domain.model.DetectionResult
import com.rootdetector.domain.model.PluginResult
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async
import kotlinx.coroutines.awaitAll
import kotlinx.coroutines.withContext

class SideChannelDetector(
    private val interruptDetector: InterruptTimingDetector = InterruptTimingDetector(),
    private val cacheDetector: CacheTimingDetector = CacheTimingDetector(),
    private val binderDetector: BinderLatencyDetector = BinderLatencyDetector(),
    private val pagefaultDetector: PagefaultMonitor = PagefaultMonitor()
) {

    suspend fun detectAll(): DetectionResult = withContext(Dispatchers.IO) {
        val deferred = listOf(
            async { runInterruptTiming() },
            async { runCacheTiming() },
            async { runBinderLatency() },
            async { runPagefault() }
        )
        val results = deferred.awaitAll()

        val anyDetected = results.any { it.detected }
        val riskScore = results.sumOf {
            when {
                it.detected && it.detail.contains("中断") -> 70
                it.detected && it.detail.contains("Cache") -> 65
                it.detected && it.detail.contains("Binder") -> 60
                it.detected && it.detail.contains("PageFault") -> 55
                else -> 0
            }
        }

        DetectionResult(
            layerResults = results,
            rootDetected = anyDetected,
            riskScore = riskScore.coerceIn(0, 100),
            reportId = "sidechannel_${System.currentTimeMillis()}"
        )
    }

    private suspend fun runInterruptTiming(): PluginResult {
        val start = System.nanoTime()
        val oldResult = interruptDetector.detect()
        val cost = System.nanoTime() - start
        return PluginResult(
            layerId = 51,
            detected = oldResult.detected,
            confidence = if (oldResult.detected) 85 else 5,
            riskScore = if (oldResult.detected) 70 else 0,
            costNs = cost,
            detail = oldResult.detail
        )
    }

    private suspend fun runCacheTiming(): PluginResult {
        val start = System.nanoTime()
        val oldResult = cacheDetector.detect()
        val cost = System.nanoTime() - start
        return PluginResult(
            layerId = 52,
            detected = oldResult.detected,
            confidence = if (oldResult.detected) 80 else 5,
            riskScore = if (oldResult.detected) 65 else 0,
            costNs = cost,
            detail = oldResult.detail
        )
    }

    private suspend fun runBinderLatency(): PluginResult {
        val start = System.nanoTime()
        val oldResult = binderDetector.detect()
        val cost = System.nanoTime() - start
        return PluginResult(
            layerId = 53,
            detected = oldResult.detected,
            confidence = if (oldResult.detected) 75 else 5,
            riskScore = if (oldResult.detected) 60 else 0,
            costNs = cost,
            detail = oldResult.detail
        )
    }

    private suspend fun runPagefault(): PluginResult {
        val start = System.nanoTime()
        val oldResult = pagefaultDetector.detect()
        val cost = System.nanoTime() - start
        return PluginResult(
            layerId = 54,
            detected = oldResult.detected,
            confidence = if (oldResult.detected) 70 else 5,
            riskScore = if (oldResult.detected) 55 else 0,
            costNs = cost,
            detail = oldResult.detail
        )
    }
}
