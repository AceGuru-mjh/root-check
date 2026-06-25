package com.rootdetector.detect

import com.rootdetector.core.security.SelfProtection
import com.rootdetector.data.datasource.NativeDataSource
import com.rootdetector.data.datasource.NativeDataSourceImpl
import com.rootdetector.data.repository.DetectionRepositoryImpl
import com.rootdetector.domain.model.DetectionLevel
import com.rootdetector.domain.model.DetectionResult
import com.rootdetector.domain.model.PluginResult
import com.rootdetector.domain.model.ThreatLevel
import com.rootdetector.domain.repository.DetectionRepository
import com.rootdetector.domain.usecase.RunDetectionUseCase

class DetectionEngine {

    private external fun nativeStartDaemon(): Boolean
    private external fun nativeRunDetection(layerMask: Int): String?
    private external fun nativeQuickDetect(): String?

    private external fun nativeDetectShamiko(): com.rootdetector.detect.DetectionResult
    private external fun nativeDetectZygiskNext(): com.rootdetector.detect.DetectionResult
    private external fun nativeDetectApatchKpm(): com.rootdetector.detect.DetectionResult
    private external fun nativeDetectKernelSUOverlay(): com.rootdetector.detect.DetectionResult
    private external fun nativeDetectLSPosed(): com.rootdetector.detect.DetectionResult

    private val nativeDataSource: NativeDataSource by lazy {
        NativeDataSourceImpl(
            nativeStartDaemon = { nativeStartDaemon() },
            nativeRunDetection = { mask -> nativeRunDetection(mask) },
            nativeQuickDetect = { nativeQuickDetect() }
        )
    }

    private val repository: DetectionRepository by lazy {
        DetectionRepositoryImpl(nativeDataSource)
    }

    val detectUseCase: RunDetectionUseCase by lazy {
        RunDetectionUseCase(repository)
    }

    companion object {
        @JvmStatic
        val available: Boolean by lazy {
            try {
                System.loadLibrary("rootguard_jni")
                true
            } catch (_: UnsatisfiedLinkError) {
                false
            }
        }

        @JvmStatic
        val antiHidingAvailable: Boolean by lazy {
            try {
                System.loadLibrary("rootdetector")
                true
            } catch (_: UnsatisfiedLinkError) {
                false
            }
        }
    }

    suspend fun detect(level: DetectionLevel): DetectionResult {
        return if (repository.isDaemonRunning()) {
            detectUseCase(level)
        } else {
            if (repository.startDaemon()) detectUseCase(level)
            else detectUseCase(DetectionLevel.QUICK)
        }
    }

    suspend fun detectWithAntiHiding(level: DetectionLevel): DetectionResult {
        val baseResult = detect(level)
        val probes = runAntiHidingProbes()
        val allResults = baseResult.layerResults + probes.flatMap { it.layerResults }
        val anyDetected = allResults.any { it.detected }
        val maxRisk = allResults.maxOfOrNull { it.riskScore } ?: 0
        return DetectionResult(
            layerResults = allResults,
            rootDetected = baseResult.rootDetected || anyDetected,
            riskScore = maxOf(baseResult.riskScore, maxRisk),
            reportId = "full_${System.currentTimeMillis()}"
        )
    }

    suspend fun runAntiHidingProbes(): List<DetectionResult> {
        if (!antiHidingAvailable) return emptyList()
        val oldResults = listOf(
            nativeDetectShamiko(),
            nativeDetectZygiskNext(),
            nativeDetectApatchKpm(),
            nativeDetectKernelSUOverlay(),
            nativeDetectLSPosed()
        )
        return oldResults.map { old ->
            val layerId = when (old.layer) {
                "对抗隐藏-SHamiko行为特征探针" -> 12
                "对抗隐藏-ZygiskNext隔离层探针" -> 13
                "对抗隐藏-APatch KPM内核补丁探针" -> 14
                "对抗隐藏-KernelSU overlayfs伪装探针" -> 15
                "对抗隐藏-LSPosed Riru双模式区分探针" -> 16
                else -> 0
            }
            DetectionResult(
                layerResults = listOf(PluginResult(
                    layerId = layerId,
                    detected = old.detected,
                    confidence = if (old.detected) 85 else 5,
                    riskScore = if (old.detected) 60 else 0,
                    costNs = 0,
                    detail = old.detail
                )),
                rootDetected = old.detected,
                riskScore = if (old.detected) 60 else 0,
                reportId = "probe_${System.currentTimeMillis()}"
            )
        }
    }

    suspend fun quickCheck(): DetectionResult {
        return detectUseCase.quick()
    }

    suspend fun ensureDaemonRunning(): Boolean {
        if (!repository.isDaemonRunning()) {
            return repository.startDaemon()
        }
        return true
    }

    fun selfCheck(): List<String> {
        val issues = mutableListOf<String>()
        if (SelfProtection.isDebuggerAttached()) issues.add("Debugger detected")
        if (SelfProtection.isPtraced()) issues.add("Process traced")
        val hooks = SelfProtection.detectHook()
        if (hooks.isNotEmpty()) issues.add("Hook libraries: ${hooks.joinToString()}")
        return issues
    }
}