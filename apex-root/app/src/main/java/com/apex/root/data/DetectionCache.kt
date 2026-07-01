package com.apex.root.data

import android.util.LruCache
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import java.util.concurrent.atomic.AtomicInteger
import java.util.concurrent.locks.ReentrantReadWriteLock
import kotlin.concurrent.read
import kotlin.concurrent.write

data class CachedResult(
    val key: String,
    val value: String,
    val numericValue: Int = 0,
    val booleanValue: Boolean = false,
    val timestampMs: Long = System.currentTimeMillis(),
    val ttlMs: Long = DEFAULT_TTL_MS
)

/**
 * 检测缓存 — 线程安全实现
 *
 * 修复：
 * 1. 使用 ReentrantReadWriteLock 保护 cache 操作
 * 2. 使用 AtomicInteger 保证统计信息原子更新
 * 3. 差异化 TTL — 不同数据类型设置不同过期时间
 */
object DetectionCache {
    private const val MAX_CACHE_SIZE = 128

    // 差异化 TTL
    const val DEFAULT_TTL_MS = 5000L           // 默认 5 秒
    const val SCAN_RESULT_TTL_MS = 30000L      // 扫描结果 30 秒
    const val RISK_SCORE_TTL_MS = 10000L       // 风险分 10 秒
    const val MEM_FINGERPRINT_TTL_MS = 60000L  // 内存指纹 60 秒
    const val ROOT_STATUS_TTL_MS = 15000L      // root 状态 15 秒

    private val cache = LruCache<String, CachedResult>(MAX_CACHE_SIZE)
    private val lock = ReentrantReadWriteLock()

    private val hitCount = AtomicInteger(0)
    private val missCount = AtomicInteger(0)

    private val _stats = MutableStateFlow(CacheStats())
    val stats: StateFlow<CacheStats> = _stats.asStateFlow()

    data class CacheStats(
        val hitCount: Int = 0,
        val missCount: Int = 0,
        val size: Int = 0
    )

    private fun updateStats() {
        _stats.value = CacheStats(
            hitCount = hitCount.get(),
            missCount = missCount.get(),
            size = lock.read { cache.size() }
        )
    }

    fun get(key: String): CachedResult? = lock.read {
        val cached = cache.get(key)
        if (cached != null) {
            val age = System.currentTimeMillis() - cached.timestampMs
            if (age < cached.ttlMs) {
                hitCount.incrementAndGet()
                updateStats()
                return@read cached
            }
            // 过期 — 需要写锁删除
            null
        } else {
            missCount.incrementAndGet()
            updateStats()
            null
        }
    }?.also {
        // 如果返回 null 是因为过期，清理它
    } ?: run {
        // 检查是否是过期导致的 null
        lock.write {
            val cached = cache.get(key)
            if (cached != null) {
                val age = System.currentTimeMillis() - cached.timestampMs
                if (age >= cached.ttlMs) {
                    cache.remove(key)
                    missCount.incrementAndGet()
                    updateStats()
                }
            }
        }
        null
    }

    fun put(key: String, value: CachedResult) = lock.write {
        cache.put(key, value)
        updateStats()
    }

    fun putString(key: String, value: String, ttlMs: Long = DEFAULT_TTL_MS) {
        put(key, CachedResult(key, value, ttlMs = ttlMs))
    }

    fun putBoolean(key: String, value: Boolean, ttlMs: Long = DEFAULT_TTL_MS) {
        put(key, CachedResult(key, if (value) "true" else "false", booleanValue = value, ttlMs = ttlMs))
    }

    fun putInt(key: String, value: Int, ttlMs: Long = DEFAULT_TTL_MS) {
        put(key, CachedResult(key, value.toString(), numericValue = value, ttlMs = ttlMs))
    }

    fun getString(key: String): String? = get(key)?.value
    fun getBoolean(key: String): Boolean? = get(key)?.booleanValue
    fun getInt(key: String): Int? = get(key)?.numericValue

    fun invalidate(key: String) = lock.write {
        cache.remove(key)
        updateStats()
    }

    fun invalidateAll() = lock.write {
        cache.evictAll()
        hitCount.set(0)
        missCount.set(0)
        updateStats()
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
