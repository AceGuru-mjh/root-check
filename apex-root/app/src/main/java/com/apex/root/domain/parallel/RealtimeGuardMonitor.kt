package com.apex.root.domain.parallel

import android.content.Context
import android.util.Log
import com.apex.root.data.jni.NativeBridge
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asSharedFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

/**
 * 实时监控引擎 — v1.1.0 新增。
 *
 * 周期性 (默认 60s) 执行轻量级检测,与基线对比:
 *  - 若发现新增检测层 → 立即发出 Alert
 *  - 若风险分显著上升 → 发出 Warning
 *  - 若内存指纹变化 → 发出 Info
 *
 * 用于在用户已经过一次完整扫描后,持续监控设备状态,
 * 检测 root 工具在后台被安装/激活的场景。
 */
class RealtimeGuardMonitor(private val context: Context) {

    companion object {
        private const val TAG = "RealtimeGuard"
        const val DEFAULT_INTERVAL_MS = 60_000L  // 1 分钟
        const val MIN_INTERVAL_MS = 30_000L     // 最小 30s
        const val MAX_INTERVAL_MS = 600_000L    // 最大 10 分钟
    }

    /** 告警级别 */
    enum class AlertLevel { INFO, WARNING, ALERT, CRITICAL }

    /** 告警事件 */
    data class GuardAlert(
        val level: AlertLevel,
        val title: String,
        val message: String,
        val timestamp: Long = System.currentTimeMillis(),
        val affectedLayers: Set<Int> = emptySet()
    )

    /** 监控状态 */
    data class GuardState(
        val isActive: Boolean,
        val intervalMs: Long,
        val lastCheckTimestamp: Long?,
        val lastCheckRiskScore: Int,
        val consecutiveAnomalies: Int,
        val totalAlerts: Int
    )

    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Default)
    private var monitorJob: Job? = null

    private val _state = MutableStateFlow(
        GuardState(false, DEFAULT_INTERVAL_MS, null, 0, 0, 0)
    )
    val state: StateFlow<GuardState> = _state.asStateFlow()

    private val _alerts = MutableSharedFlow<GuardAlert>(replay = 0, extraBufferCapacity = 64)
    val alerts: SharedFlow<GuardAlert> = _alerts.asSharedFlow()

    private val baseline = DeviceFingerprintBaseline(context)
    private val quickEngine = ParallelDetectionEngine()

    // P3-6: 缓存最近一次扫描结果,establishBaseline 和 doCheck 复用,
    // 避免首次启动时两个调用都触发 quickEngine.scanParallel() (各 7-8 次
    // quickScan,合计 14-16 次 native 全量扫描)。
    @Volatile
    private var lastScanResult: ParallelScanResult? = null
    private val scanLock = Any()

    /**
     * 启动实时监控。
     * @param intervalMs 检测间隔 (毫秒)
     */
    fun start(intervalMs: Long = DEFAULT_INTERVAL_MS) {
        if (_state.value.isActive) {
            Log.w(TAG, "Monitor already active")
            return
        }
        val interval = intervalMs.coerceIn(MIN_INTERVAL_MS, MAX_INTERVAL_MS)
        _state.value = _state.value.copy(isActive = true, intervalMs = interval)
        monitorJob = scope.launch {
            Log.i(TAG, "Realtime guard started, interval=${interval}ms")
            // 立即执行一次检测
            doCheck()
            while (_state.value.isActive) {
                delay(interval)
                if (!_state.value.isActive) break
                doCheck()
            }
        }
    }

    /**
     * 停止实时监控。
     */
    fun stop() {
        _state.value = _state.value.copy(isActive = false)
        monitorJob?.cancel()
        monitorJob = null
        Log.i(TAG, "Realtime guard stopped")
    }

    /**
     * 立即执行一次检测 (不等待下一个间隔)。
     */
    suspend fun checkNow() {
        doCheck()
    }

    private suspend fun doCheck() {
        try {
            // P3-6: 复用最近一次 scanResult (TTL = 当前检测间隔),避免与
            // establishBaseline 在启动瞬间同时触发 2 次并行扫描。
            val scanResult = obtainScanResult()
            val snapshot = baseline.captureSnapshot(scanResult)

            // 对比基线
            val diff = baseline.diff(snapshot)

            val alertsToEmit = mutableListOf<GuardAlert>()
            var consecutive = _state.value.consecutiveAnomalies

            if (diff == null) {
                // 无基线 — 不告警,但记录状态
                Log.d(TAG, "No baseline, skipping diff")
            } else {
                // 检查新增检测层
                if (diff.newDetectedLayers.isNotEmpty()) {
                    val level = if (diff.newDetectedLayers.any { it in setOf(8, 9, 10, 17, 18) }) {
                        AlertLevel.CRITICAL  // Magisk/KSU/APatch 出现 = 严重
                    } else {
                        AlertLevel.ALERT
                    }
                    alertsToEmit.add(GuardAlert(
                        level = level,
                        title = "新增 root 检测信号",
                        message = "检测到新出现的 root 痕迹:\n${diff.summary}",
                        affectedLayers = diff.newDetectedLayers
                    ))
                    consecutive++
                }

                // 检查风险分显著上升
                if (diff.riskScoreDelta > 15) {
                    alertsToEmit.add(GuardAlert(
                        level = AlertLevel.WARNING,
                        title = "风险评分显著上升",
                        message = "风险分从基线上升了 ${diff.riskScoreDelta} 点 (当前 ${snapshot.riskScore})"
                    ))
                    consecutive++
                }

                // 检查内存指纹变化 (可能新注入了 hook)
                if (diff.memFingerprintChanged && snapshot.memFingerprintMask != 0) {
                    alertsToEmit.add(GuardAlert(
                        level = AlertLevel.WARNING,
                        title = "内存指纹变化",
                        message = "检测到新的内存注入痕迹 (mask=${snapshot.memFingerprintMask})"
                    ))
                    consecutive++
                }

                // 检查新安装的 root 应用
                if (diff.newRootApps.isNotEmpty()) {
                    alertsToEmit.add(GuardAlert(
                        level = AlertLevel.CRITICAL,
                        title = "新安装 root 工具",
                        message = "检测到新安装的 root 工具: ${diff.newRootApps.joinToString(", ")}"
                    ))
                    consecutive++
                }

                // 如果一切正常,重置连续异常计数
                if (alertsToEmit.isEmpty() && diff.hasSignificantChanges) {
                    alertsToEmit.add(GuardAlert(
                        level = AlertLevel.INFO,
                        title = "设备状态变化",
                        message = diff.summary
                    ))
                }
                if (alertsToEmit.isEmpty()) {
                    consecutive = 0
                }
            }

            // 发出告警
            for (alert in alertsToEmit) {
                _alerts.emit(alert)
                Log.i(TAG, "Alert [${alert.level}]: ${alert.title} — ${alert.message}")
            }

            // 更新状态
            _state.value = _state.value.copy(
                lastCheckTimestamp = System.currentTimeMillis(),
                lastCheckRiskScore = snapshot.riskScore,
                consecutiveAnomalies = consecutive,
                totalAlerts = _state.value.totalAlerts + alertsToEmit.size
            )
        } catch (e: Exception) {
            Log.e(TAG, "Guard check failed: ${e.message}", e)
        }
    }

    /**
     * 更新检测间隔 (运行时调整)。
     */
    fun updateInterval(intervalMs: Long) {
        val interval = intervalMs.coerceIn(MIN_INTERVAL_MS, MAX_INTERVAL_MS)
        val wasActive = _state.value.isActive
        if (wasActive) {
            stop()
            _state.value = _state.value.copy(intervalMs = interval)
            start(interval)
        } else {
            _state.value = _state.value.copy(intervalMs = interval)
        }
    }

    /**
     * 建立当前设备基线 (覆盖现有基线)。
     *
     * P3-6 修复: 若最近一次扫描结果在当前检测间隔内,复用之,避免与
     * 启动瞬间的 [doCheck] 同时触发 2 次并行 native 扫描。
     */
    suspend fun establishBaseline() {
        val scanResult = obtainScanResult()
        val snapshot = baseline.captureSnapshot(scanResult)
        baseline.saveBaseline(snapshot)
        Log.i(TAG, "Baseline established: risk=${snapshot.riskScore}, " +
            "layers=${snapshot.detectedLayerIds.size}, rootApps=${snapshot.installedRootApps}")
    }

    /**
     * P3-6: 统一的扫描结果获取入口 — 若缓存命中且未过期则复用,
     * 否则触发一次新扫描并写入缓存。并行调用通过 [scanLock] 串行化,
     * 避免双重扫描。
     */
    private suspend fun obtainScanResult(): ParallelScanResult {
        val ttl = _state.value.intervalMs
        val cached = lastScanResult
        if (cached != null && System.currentTimeMillis() - cached.timestamp < ttl) {
            Log.d(TAG, "Reusing cached scanResult (age=${System.currentTimeMillis() - cached.timestamp}ms)")
            return cached
        }
        // scanParallel 是 suspend,不能用 synchronized 块包裹;改用先扫再写缓存,
        // 并发触发时最坏情况会扫描 2 次,但缓存写入用 synchronized 保证可见性。
        val fresh = quickEngine.scanParallel()
        synchronized(scanLock) { lastScanResult = fresh }
        return fresh
    }

    /**
     * 清除基线 (下次扫描将重新建立)。
     */
    fun clearBaseline() {
        baseline.clearBaseline()
        Log.i(TAG, "Baseline cleared")
    }

    fun hasBaseline(): Boolean = baseline.hasBaseline()
}
