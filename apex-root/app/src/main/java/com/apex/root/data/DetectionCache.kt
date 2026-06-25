package com.apex.root.data

import android.util.LruCache
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow

data class CachedResult(
    val key: String,
    val value: String,
    val numericValue: Int = 0,
    val booleanValue: Boolean = false,
    val timestampMs: Long = System.currentTimeMillis()
)

object DetectionCache {
    private const val MAX_CACHE_SIZE = 128
    private const val CACHE_TTL_MS = 5000L // 5 seconds for fast detection

    private val cache = LruCache<String, CachedResult>(MAX_CACHE_SIZE)

    private val _stats = MutableStateFlow(CacheStats())
    val stats: StateFlow<CacheStats> = _stats.asStateFlow()

    data class CacheStats(
        val hitCount: Int = 0,
        val missCount: Int = 0,
        val size: Int = 0
    )

    fun get(key: String): CachedResult? {
        val cached = cache.get(key)
        if (cached != null) {
            val age = System.currentTimeMillis() - cached.timestampMs
            if (age < CACHE_TTL_MS) {
                _stats.value = _stats.value.copy(
                    hitCount = _stats.value.hitCount + 1,
                    size = cache.size()
                )
                return cached
            }
            // Expired
            cache.remove(key)
        }
        _stats.value = _stats.value.copy(
            missCount = _stats.value.missCount + 1,
            size = cache.size()
        )
        return null
    }

    fun put(key: String, value: CachedResult) {
        cache.put(key, value)
        _stats.value = _stats.value.copy(size = cache.size())
    }

    fun putString(key: String, value: String) {
        put(key, CachedResult(key, value))
    }

    fun putBoolean(key: String, value: Boolean) {
        put(key, CachedResult(key, if (value) "true" else "false", booleanValue = value))
    }

    fun putInt(key: String, value: Int) {
        put(key, CachedResult(key, value.toString(), numericValue = value))
    }

    fun getString(key: String): String? = get(key)?.value

    fun getBoolean(key: String): Boolean? = get(key)?.booleanValue

    fun getInt(key: String): Int? = get(key)?.numericValue

    fun invalidate(key: String) {
        cache.remove(key)
    }

    fun invalidateAll() {
        cache.evictAll()
        _stats.value = CacheStats(size = 0)
    }

    // Pre-computed detection keys
    const val KEY_QUICK_SCAN = "quick_scan"
    const val KEY_RISK_SCORE = "risk_score"
    const val KEY_IS_ROOTED = "is_rooted"
    const val KEY_MEM_FINGERPRINT = "mem_fingerprint"
    const val KEY_SELINUX_STATUS = "selinux_status"
    const val KEY_SHAMIKO = "shamiko"
    const val KEY_ZYGISK_NEXT = "zygisk_next"
    const val KEY_MEMORY_REPORT = "memory_report"
    const val KEY_SELINUX_REPORT = "selinux_report"
    const val KEY_SHAMIKO_REPORT = "shamiko_report"
    const val KEY_ZYGISKNEXT_REPORT = "zygisknext_report"
}
