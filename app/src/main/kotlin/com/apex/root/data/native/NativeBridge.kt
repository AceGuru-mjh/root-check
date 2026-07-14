package com.apex.root.data.native

import android.util.Log
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * Native Bridge for Root Detection
 * 
 * 提供 JNI 接口调用 C++ 原生检测逻辑
 * 所有检测方法都在 Native 层执行，防止被 Hook 篡改
 */
object NativeBridge {
    
    companion object {
        private const val TAG = "NativeBridge"
        
        init {
            try {
                System.loadLibrary("apexroot")
                Log.i(TAG, "Native library loaded successfully")
            } catch (e: UnsatisfiedLinkError) {
                Log.e(TAG, "Failed to load native library: ${e.message}")
            }
        }
    }
    
    /**
     * 检测设备是否 Root
     * @return RootDetectionResult 包含检测结果和置信度
     */
    suspend fun isDeviceRooted(): RootDetectionResult = withContext(Dispatchers.Default) {
        try {
            val result = nativeIsDeviceRooted()
            RootDetectionResult(
                isRooted = result.first,
                confidence = result.second,
                detectedMethods = emptyList(),
                timestamp = System.currentTimeMillis()
            )
        } catch (e: Exception) {
            Log.e(TAG, "Native detection failed: ${e.message}")
            RootDetectionResult(
                isRooted = false,
                confidence = 0.0f,
                detectedMethods = emptyList(),
                timestamp = System.currentTimeMillis(),
                error = e.message
            )
        }
    }
    
    /**
     * 检测 Busybox 安装
     * @return true 如果检测到 Busybox
     */
    suspend fun hasBusybox(): Boolean = withContext(Dispatchers.Default) {
        try {
            nativeHasBusybox()
        } catch (e: Exception) {
            Log.e(TAG, "Busybox detection failed: ${e.message}")
            false
        }
    }
    
    /**
     * 检测危险应用
     * @return 危险应用包名列表
     */
    suspend fun detectDangerousApps(): List<String> = withContext(Dispatchers.Default) {
        try {
            nativeDetectDangerousApps().toList()
        } catch (e: Exception) {
            Log.e(TAG, "Dangerous apps detection failed: ${e.message}")
            emptyList()
        }
    }
    
    /**
     * 检查系统属性
     * @param propName 属性名
     * @return 属性值
     */
    suspend fun getSystemProperty(propName: String): String = withContext(Dispatchers.Default) {
        try {
            nativeGetSystemProperty(propName) ?: ""
        } catch (e: Exception) {
            Log.e(TAG, "Get system property failed: ${e.message}")
            ""
        }
    }
    
    // Native 方法声明
    private external fun nativeIsDeviceRooted(): Pair<Boolean, Float>
    private external fun nativeHasBusybox(): Boolean
    private external fun nativeDetectDangerousApps(): Array<String>
    private external fun nativeGetSystemProperty(propName: String): String?
}

/**
 * Root 检测结果数据类
 */
data class RootDetectionResult(
    val isRooted: Boolean,
    val confidence: Float,
    val detectedMethods: List<String>,
    val timestamp: Long,
    val error: String? = null
)
