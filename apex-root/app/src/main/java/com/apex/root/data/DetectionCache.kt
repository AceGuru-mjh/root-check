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
    val ttlMs: Long = 5000L
)

/**
 * 缓存类型 — 差异化 TTL
 */
enum class CacheType(val ttlMs: Long) {
    QUICK_SCAN(5_000),           // 快速检测：5 秒
    RISK_SCORE(60_000),          // 风险评分：60 秒
    DEVICE_FINGERPRINT(300_000), // 设备指纹：5 分钟
    SELINUX_STATUS(30_000),      // SELinux 状态：30 秒
    MEMORY_REPORT(10_000),       // 内存报告：10 秒
    ROOT_STATUS(15_000),         // Root 状态：15 秒
    DEFAULT(5_000);              // 默认：5 秒
}

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

    // v1.0.2 P0-1 修复: 将读+删合并到单个 write 锁内,消除竞态条件
    // 旧实现: 读锁内发现过期→返回null→释放读锁→获取写锁删除。
    // 多线程同时访问同一过期 key 时,每个线程都会走一遍写锁删除,且在读锁释放
    // 到写锁获取之间,另一个线程可能已经 put 了新值却被误删。
    // 新实现: 直接用 write 锁完成 get+过期检查+remove,保证原子性。
    fun get(key: String): CachedResult? = lock.write {
        val cached = cache.get(key)
        if (cached != null) {
            val age = System.currentTimeMillis() - cached.timestampMs
            if (age < cached.ttlMs) {
                // 未过期 — 命中
                hitCount.incrementAndGet()
                updateStats()
                return@write cached
            } else {
                // 过期 — 在同一个写锁内删除,不会被其他线程重复删除
                cache.remove(key)
                missCount.incrementAndGet()
                updateStats()
                return@write null
            }
        } else {
            missCount.incrementAndGet()
            updateStats()
            return@write null
        }
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

    /** 按缓存类型存储 */
    fun putWithType(key: String, value: String, type: CacheType) {
        put(key, CachedResult(key, value, ttlMs = type.ttlMs))
    }

    /** 按缓存类型存储 Boolean */
    fun putWithType(key: String, value: Boolean, type: CacheType) {
        put(key, CachedResult(key, if (value) "true" else "false", booleanValue = value, ttlMs = type.ttlMs))
    }

    /** 按缓存类型存储 Int */
    fun putWithType(key: String, value: Int, type: CacheType) {
        put(key, CachedResult(key, value.toString(), numericValue = value, ttlMs = type.ttlMs))
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
