package com.rootdetector.cache

import android.content.Context
import android.content.SharedPreferences
import com.google.gson.Gson
import com.google.gson.reflect.TypeToken
import com.rootdetector.detect.DetectionMode
import com.rootdetector.detect.DetectionResult
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import java.security.MessageDigest

/**
 * 检测结果缓存管理器
 * 实现增量检测机制，只检测变化的部分
 */
class DetectionCache private constructor(context: Context) {
    
    private val prefs: SharedPreferences = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
    private val gson = Gson()
    private val mutex = Mutex()
    
    companion object {
        private const val PREFS_NAME = "detection_cache"
        private const val KEY_LAST_RESULTS = "last_results"
        private const val KEY_LAST_SCAN_TIME = "last_scan_time"
        private const val KEY_LAST_MODE = "last_mode"
        private const val KEY_SYSTEM_FINGERPRINT = "system_fingerprint"
        private const val CACHE_EXPIRY_MS = 5 * 60 * 1000L // 5分钟缓存过期时间
        
        @Volatile
        private var instance: DetectionCache? = null
        
        fun getInstance(context: Context): DetectionCache {
            return instance ?: synchronized(this) {
                instance ?: DetectionCache(context.applicationContext).also { instance = it }
            }
        }
    }
    
    /**
     * 缓存数据类
     */
    data class CacheData(
        val results: List<DetectionResult>,
        val timestamp: Long,
        val mode: DetectionMode,
        val fingerprint: String
    )
    
    /**
     * 获取缓存的检测结果
     * @param mode 检测模式
     * @return 缓存的结果，如果缓存无效则返回 null
     */
    suspend fun getCachedResults(mode: DetectionMode): List<DetectionResult>? = mutex.withLock {
        val cacheData = loadCacheData() ?: return@withLock null
        
        // 检查缓存是否过期
        if (System.currentTimeMillis() - cacheData.timestamp > CACHE_EXPIRY_MS) {
            clearCache()
            return@withLock null
        }
        
        // 检查检测模式是否匹配
        if (cacheData.mode != mode) {
            return@withLock null
        }
        
        // 检查系统指纹是否变化
        val currentFingerprint = generateSystemFingerprint()
        if (cacheData.fingerprint != currentFingerprint) {
            return@withLock null
        }
        
        cacheData.results
    }
    
    /**
     * 保存检测结果到缓存
     * @param results 检测结果
     * @param mode 检测模式
     */
    suspend fun saveResults(results: List<DetectionResult>, mode: DetectionMode) = mutex.withLock {
        val fingerprint = generateSystemFingerprint()
        val cacheData = CacheData(
            results = results,
            timestamp = System.currentTimeMillis(),
            mode = mode,
            fingerprint = fingerprint
        )
        
        val json = gson.toJson(cacheData)
        prefs.edit()
            .putString(KEY_LAST_RESULTS, json)
            .putLong(KEY_LAST_SCAN_TIME, cacheData.timestamp)
            .putString(KEY_LAST_MODE, mode.name)
            .putString(KEY_SYSTEM_FINGERPRINT, fingerprint)
            .apply()
    }
    
    /**
     * 清除缓存
     */
    suspend fun clearCache() = mutex.withLock {
        prefs.edit().clear().apply()
    }
    
    /**
     * 执行增量检测
     * 只检测发生变化的检测项
     * @param mode 检测模式
     * @param detector 检测器函数
     * @return 检测结果列表
     */
    suspend fun detectIncrementally(
        mode: DetectionMode,
        detector: suspend () -> List<DetectionResult>
    ): List<DetectionResult> = mutex.withLock {
        val cachedData = loadCacheData()
        
        // 如果没有缓存或缓存已过期，执行完整检测
        if (cachedData == null || 
            System.currentTimeMillis() - cachedData.timestamp > CACHE_EXPIRY_MS ||
            cachedData.mode != mode) {
            val results = detector()
            saveResults(results, mode)
            return@withLock results
        }
        
        // 检查系统指纹是否变化
        val currentFingerprint = generateSystemFingerprint()
        if (cachedData.fingerprint != currentFingerprint) {
            // 系统发生变化，执行完整检测
            val results = detector()
            saveResults(results, mode)
            return@withLock results
        }
        
        // 系统未变化，返回缓存结果
        cachedData.results
    }
    
    /**
     * 加载缓存数据
     */
    private fun loadCacheData(): CacheData? {
        val json = prefs.getString(KEY_LAST_RESULTS, null) ?: return null
        
        return try {
            gson.fromJson(json, CacheData::class.java)
        } catch (e: Exception) {
            null
        }
    }
    
    /**
     * 生成系统指纹
     * 用于检测系统环境是否发生变化
     */
    private fun generateSystemFingerprint(): String {
        val data = buildString {
            // 收集系统关键属性
            append(android.os.Build.FINGERPRINT)
            append(android.os.Build.MODEL)
            append(android.os.Build.MANUFACTURER)
            append(android.os.Build.PRODUCT)
            append(android.os.Build.HARDWARE)
            append(android.os.Build.BOARD)
            
            // 添加一些运行时特征
            append(System.getProperty("os.arch"))
            append(System.getProperty("java.vm.version"))
            
            // 添加关键目录状态
            append(checkKeyDirectory("/system"))
            append(checkKeyDirectory("/data"))
            append(checkKeyDirectory("/vendor"))
        }
        
        return hashString(data)
    }
    
    /**
     * 检查关键目录状态
     */
    private fun checkKeyDirectory(path: String): String {
        return try {
            val file = java.io.File(path)
            "${file.exists()}:${file.canRead()}:${file.lastModified()}"
        } catch (e: Exception) {
            "error"
        }
    }
    
    /**
     * 对字符串进行 SHA-256 哈希
     */
    private fun hashString(input: String): String {
        val bytes = MessageDigest.getInstance("SHA-256")
            .digest(input.toByteArray())
        return bytes.joinToString("") { "%02x".format(it) }
    }
    
    /**
     * 获取缓存状态信息
     */
    fun getCacheStatus(): CacheStatus {
        val cacheData = loadCacheData()
        
        return if (cacheData != null) {
            val age = System.currentTimeMillis() - cacheData.timestamp
            CacheStatus(
                isValid = age <= CACHE_EXPIRY_MS,
                ageMs = age,
                mode = cacheData.mode,
                resultCount = cacheData.results.size
            )
        } else {
            CacheStatus(
                isValid = false,
                ageMs = 0,
                mode = null,
                resultCount = 0
            )
        }
    }
    
    /**
     * 缓存状态数据类
     */
    data class CacheStatus(
        val isValid: Boolean,
        val ageMs: Long,
        val mode: DetectionMode?,
        val resultCount: Int
    )
}
