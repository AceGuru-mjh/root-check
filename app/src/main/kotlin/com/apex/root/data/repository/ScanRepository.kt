package com.apex.root.data.repository

import android.content.Context
import com.apex.root.BuildConfig
import com.apex.root.data.database.ApexDatabase
import com.apex.root.domain.model.DetectionMethod
import com.apex.root.domain.model.DetectionLayer
import com.apex.root.domain.model.RootDetectionResult
import com.apex.root.domain.model.RiskLevel
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.withContext

/**
 * Root 检测仓库 - 集成 Native 层
 */
class ScanRepository(private val context: Context) {
    
    private val database by lazy { ApexDatabase.getDatabase(context) }
    private val guardEventRepository = GuardEventRepository(database.guardEventDao())
    
    // Native bridge (待实现 C++ 层后启用)
    // private val nativeBridge = NativeBridge()
    
    private val _scanProgress = MutableStateFlow(0f)
    val scanProgress: StateFlow<Float> = _scanProgress.asStateFlow()
    
    private val _isScanning = MutableStateFlow(false)
    val isScanning: StateFlow<Boolean> = _isScanning.asStateFlow()
    
    /**
     * 执行完整扫描
     */
    suspend fun performFullScan(): RootDetectionResult = withContext(Dispatchers.Default) {
        _isScanning.value = true
        _scanProgress.value = 0f
        
        try {
            val detectedMethods = mutableListOf<DetectionMethod>()
            var maxConfidence = 0f
            
            // L1: System Property Checks
            _scanProgress.value = 0.1f
            val l1Results = checkLayer1Properties()
            detectedMethods.addAll(l1Results)
            maxConfidence = maxOf(maxConfidence, l1Results.maxOfOrNull { it.confidence } ?: 0f)
            
            // L4: Binary File Presence
            _scanProgress.value = 0.3f
            val l4Results = checkLayer4Binaries()
            detectedMethods.addAll(l4Results)
            maxConfidence = maxOf(maxConfidence, l4Results.maxOfOrNull { it.confidence } ?: 0f)
            
            // L6: Native Syscall Tests
            _scanProgress.value = 0.5f
            val l6Results = checkLayer6Native()
            detectedMethods.addAll(l6Results)
            maxConfidence = maxOf(maxConfidence, l6Results.maxOfOrNull { it.confidence } ?: 0f)
            
            // L8: Behavior Analysis
            _scanProgress.value = 0.7f
            val l8Results = checkLayer8Behavior()
            detectedMethods.addAll(l8Results)
            maxConfidence = maxOf(maxConfidence, l8Results.maxOfOrNull { it.confidence } ?: 0f)
            
            // L9-L12: Advanced Detection (简化实现)
            _scanProgress.value = 0.9f
            val advancedResults = checkAdvancedLayers()
            detectedMethods.addAll(advancedResults)
            maxConfidence = maxOf(maxConfidence, advancedResults.maxOfOrNull { it.confidence } ?: 0f)
            
            _scanProgress.value = 1.0f
            
            // 计算最终风险等级
            val riskLevel = calculateRiskLevel(maxConfidence, detectedMethods.size)
            
            RootDetectionResult(
                isRooted = maxConfidence > 0.3f || detectedMethods.size >= 3,
                confidence = maxConfidence,
                detectedMethods = detectedMethods,
                riskLevel = riskLevel
            )
        } finally {
            _isScanning.value = false
        }
    }
    
    /**
     * L1: System Property Checks
     */
    private fun checkLayer1Properties(): List<DetectionMethod> {
        val methods = mutableListOf<DetectionMethod>()
        
        // 检查 build.prop 中的常见 root 标记
        val properties = mapOf(
            "ro.debuggable" to "1",
            "ro.secure" to "0",
            "ro.build.type" to "userdebug",
            "ro.build.tags" to "test-keys"
        )
        
        properties.forEach { (key, expectedValue) ->
            val actualValue = getSystemProperty(key)
            if (actualValue == expectedValue) {
                methods.add(
                    DetectionMethod(
                        id = "L1_$key",
                        name = "Property: $key",
                        layer = DetectionLayer.L1_PROP,
                        triggered = true,
                        details = "$key=$actualValue (expected: ${!expectedValue})"
                    )
                )
            }
        }
        
        return methods
    }
    
    /**
     * L4: Binary File Presence
     */
    private fun checkLayer4Binaries(): List<DetectionMethod> {
        val methods = mutableListOf<DetectionMethod>()
        val commonBinaries = listOf("su", "magisk", "supersu", "busybox")
        
        commonBinaries.forEach { binary ->
            val paths = listOf(
                "/system/bin/$binary",
                "/system/xbin/$binary",
                "/sbin/$binary",
                "/data/local/tmp/$binary"
            )
            
            paths.forEach { path ->
                if (fileExists(path)) {
                    methods.add(
                        DetectionMethod(
                            id = "L4_$binary",
                            name = "Binary: $binary",
                            layer = DetectionLayer.L4_BINARY,
                            triggered = true,
                            details = "Found at $path"
                        )
                    )
                }
            }
        }
        
        return methods
    }
    
    /**
     * L6: Native Syscall Tests
     */
    private fun checkLayer6Native(): List<DetectionMethod> {
        val methods = mutableListOf<DetectionMethod>()
        
        // TODO: 调用 Native Bridge 进行 syscall 测试
        // if (nativeBridge.isDeviceRooted()) { ... }
        
        return methods
    }
    
    /**
     * L8: Behavior Analysis
     */
    private fun checkLayer8Behavior(): List<DetectionMethod> {
        val methods = mutableListOf<DetectionMethod>()
        
        // TODO: 实现行为分析检测
        
        return methods
    }
    
    /**
     * L9-L12: Advanced Detection
     */
    private fun checkAdvancedLayers(): List<DetectionMethod> {
        val methods = mutableListOf<DetectionMethod>()
        
        // TODO: 实现高级检测层
        
        return methods
    }
    
    /**
     * 获取系统属性 (通过 getprop 命令)
     */
    private fun getSystemProperty(key: String): String? {
        return try {
            val process = Runtime.getRuntime().exec("getprop $key")
            process.inputStream.bufferedReader().use { it.readText().trim() }
        } catch (e: Exception) {
            null
        }
    }
    
    /**
     * 检查文件是否存在
     */
    private fun fileExists(path: String): Boolean {
        return try {
            java.io.File(path).exists()
        } catch (e: Exception) {
            false
        }
    }
    
    /**
     * 计算风险等级
     */
    private fun calculateRiskLevel(confidence: Float, methodCount: Int): RiskLevel {
        return when {
            confidence > 0.9f || methodCount >= 8 -> RiskLevel.CRITICAL
            confidence > 0.7f || methodCount >= 5 -> RiskLevel.HIGH
            confidence > 0.5f || methodCount >= 3 -> RiskLevel.MEDIUM
            confidence > 0.3f || methodCount >= 1 -> RiskLevel.LOW
            else -> RiskLevel.NONE
        }
    }
    
    /**
     * 重置扫描进度
     */
    fun resetProgress() {
        _scanProgress.value = 0f
    }
}
