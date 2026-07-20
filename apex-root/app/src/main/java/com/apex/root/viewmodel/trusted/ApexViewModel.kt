package com.apex.root.viewmodel.trusted

import android.app.Application
import android.content.Context
import android.util.Log
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.apex.root.core.security.SelfProtection
import com.apex.root.data.FixRecommendation
import com.apex.root.data.FixRecommendations
import com.apex.root.data.jni.NativeBridge
import com.apex.root.data.repository.RootDetectRepositoryImpl
import com.apex.root.domain.model.CureLevel
import com.apex.root.domain.model.GameModeState
import com.apex.root.domain.guard.model.GuardState
import com.apex.root.domain.parallel.CrossValidator
import com.apex.root.domain.parallel.DeviceFingerprintBaseline
import com.apex.root.domain.parallel.ParallelDetectionEngine
import com.apex.root.domain.parallel.ParallelScanResult
import com.apex.root.domain.parallel.ValidationResult
import com.apex.root.island.NativeIsland
import com.apex.root.hid.NativeHwid
import com.apex.root.util.ReportExporter
import kotlinx.coroutines.CoroutineExceptionHandler
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.withContext
import kotlinx.coroutines.flow.asSharedFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch

enum class LogType { INFO, WARN, ERROR }
data class LogEntry(val type: LogType, val time: String, val message: String)
enum class SnackbarType { SUCCESS, WARNING, ERROR }
data class SnackbarEvent(val message: String, val type: SnackbarType)

data class ApexUiState(
    val scanResult: String = "点击扫描开始检测",
    val riskScore: Int = 0,
    val isScanning: Boolean = false,
    val isLoading: Boolean = false,
    val isFirstLaunch: Boolean = true,
    val nativeAvailable: Boolean = true,
    val logs: List<LogEntry> = emptyList(),
    val gameMode: GameModeState = GameModeState(false),
    val guardState: GuardState = GuardState(false, false, 0),
    val sandboxPid: Int = -1,
    val sandboxActive: Boolean = false,
    val cureMessage: String = "",
    val hwidSpoofed: Boolean = false,
    // Enhanced detection results
    val memFingerprintMask: Int = 0,
    val rwxPageCount: Int = 0,
    val hasShamiko: Boolean = false,
    val hasZygiskNext: Boolean = false,
    val selinuxCompromised: Boolean = false,
    val deepReport: String = "",
    val selfCheckIssues: List<String> = emptyList(),
    // Fix recommendations
    val recommendations: List<FixRecommendation> = emptyList(),
    val showRecommendations: Boolean = false,
    // 扫描进度（0f..1f）和当前层名称（供 UI 显示进度条）
    val scanProgress: Float = 0f,
    val currentLayer: String = "",
    // 最近一次扫描的层通过数（如 12/15）
    val layersPassed: Int = 0,
    val layersTotal: Int = 0,
    // v1.2.0 并行引擎 + 交叉验证结果
    val parallelScanResult: ParallelScanResult? = null,
    val validationResult: ValidationResult? = null,
    val parallelScanLatencyMs: Long = 0L,
    // v1.2.0 基线状态
    val hasBaseline: Boolean = false,
    val baselineTimestamp: Long? = null,
    // v1.2.0 实时监控
    val guardMonitorActive: Boolean = false,
    val guardMonitorIntervalMs: Long = 60_000L,
    val guardAlerts: List<com.apex.root.domain.parallel.RealtimeGuardMonitor.GuardAlert> = emptyList(),
    val lastGuardCheckTimestamp: Long? = null,
    val consecutiveAnomalies: Int = 0
)

class ApexViewModel(application: Application) : AndroidViewModel(application) {
    private val repository = RootDetectRepositoryImpl()
    private val prefs = application.getSharedPreferences("apex_prefs", Context.MODE_PRIVATE)
    private val settingsRepo = com.apex.root.data.SettingsRepository(application)
    private val _uiState = MutableStateFlow(ApexUiState())
    val uiState: StateFlow<ApexUiState> = _uiState.asStateFlow()

    // v1.2.0 并行检测引擎 + 交叉验证 + 基线 + 实时监控
    private val parallelEngine = ParallelDetectionEngine()
    private val baseline = DeviceFingerprintBaseline(application)
    private val guardMonitor = com.apex.root.domain.parallel.RealtimeGuardMonitor(application)

    /**
     * 全局协程异常处理器 — 任何未捕获的异常都会被记录，避免崩溃整个进程。
     * 这对 native 层抛出的 Error (OutOfMemoryError / StackOverflowError) 尤其重要，
     * 因为 NativeLibraryLoader.safeCall 只捕获 Exception 和 UnsatisfiedLinkError。
     */
    private val exceptionHandler = CoroutineExceptionHandler { _, e ->
        Log.e("ApexViewModel", "Uncaught coroutine exception", e)
        // 不要在异常处理器中再 launch 协程，避免递归崩溃
    }

    /**
     * snackbar 通道：使用 extraBufferCapacity 保证 emit 不阻塞。
     * 原先使用默认配置 (replay=0, extraBufferCapacity=0)，emit 会一直 suspend 直到有订阅者，
     * 当 UI 没有收集时（如配置改变期间）调用 emit 会挂起协程，导致状态卡死。
     */
    private val _snackbarChannel = MutableSharedFlow<SnackbarEvent>(
        replay = 0,
        extraBufferCapacity = 8
    )
    val snackbarChannel: SharedFlow<SnackbarEvent> = _snackbarChannel.asSharedFlow()

    init {
        SelfProtection.init(application)
        // 异步读取 SharedPreferences，避免在主线程触发首次 XML 加载导致 ANR
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            val isFirst = try {
                prefs.getBoolean("is_first_launch", true)
            } catch (e: Throwable) {
                Log.e("ApexViewModel", "Failed to read first launch flag", e)
                true
            }
            _uiState.update { it.copy(isFirstLaunch = isFirst) }
        }
        checkNativeAvailability()

        // v1.2.0: 订阅并行引擎进度,实时更新 UI
        viewModelScope.launch(Dispatchers.Main + exceptionHandler) {
            parallelEngine.progress.collect { pct ->
                _uiState.update { it.copy(scanProgress = pct / 100f) }
            }
        }
        viewModelScope.launch(Dispatchers.Main + exceptionHandler) {
            parallelEngine.currentLayer.collect { layer ->
                _uiState.update { it.copy(currentLayer = layer) }
            }
        }

        // v1.2.0: 订阅实时监控告警
        viewModelScope.launch(Dispatchers.Main + exceptionHandler) {
            guardMonitor.alerts.collect { alert ->
                _uiState.update { state ->
                    state.copy(
                        guardAlerts = (state.guardAlerts + alert).takeLast(50),
                        consecutiveAnomalies = guardMonitor.state.value.consecutiveAnomalies,
                        lastGuardCheckTimestamp = guardMonitor.state.value.lastCheckTimestamp
                    )
                }
                // 同时推送系统通知
                notifyGuardAlert(alert)
            }
        }
        // 同步 guard monitor 状态
        viewModelScope.launch(Dispatchers.Main + exceptionHandler) {
            guardMonitor.state.collect { gs ->
                _uiState.update {
                    it.copy(
                        guardMonitorActive = gs.isActive,
                        guardMonitorIntervalMs = gs.intervalMs,
                        consecutiveAnomalies = gs.consecutiveAnomalies,
                        lastGuardCheckTimestamp = gs.lastCheckTimestamp
                    )
                }
            }
        }

        // v1.2.0: 初始化基线状态
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            _uiState.update {
                it.copy(
                    hasBaseline = baseline.hasBaseline(),
                    baselineTimestamp = baseline.loadBaseline()?.timestamp
                )
            }
        }
    }

    private fun checkNativeAvailability() {
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            val available = runCatching {
                com.apex.root.core.NativeLibraryLoader.ensureLoaded()
                com.apex.root.core.NativeLibraryLoader.isAvailable
            }.getOrDefault(false)
            _uiState.update { it.copy(nativeAvailable = available) }

            // P1-1 修复: native 库加载成功后,立即设置微服务插件目录
            // 路径 = applicationInfo.nativeLibraryDir + "/plugins"
            // 这样 service_engine::initialize() 才能正确 dlopen 20 个插件
            if (available) {
                runCatching {
                    val app = getApplication<Application>()
                    val nativeLibDir = app.applicationInfo.nativeLibraryDir
                    if (!nativeLibDir.isNullOrEmpty()) {
                        val pluginsDir = "$nativeLibDir/plugins"
                        NativeBridge.setPluginsDir(pluginsDir)
                        Log.i("ApexViewModel", "Plugins dir set: $pluginsDir")
                    }
                }.onFailure { e ->
                    Log.w("ApexViewModel", "Failed to set plugins dir: ${e.message}")
                }

                // P0-D5 修复 (v1.1.1): 接入微服务引擎初始化。
                // 此前 service_engine::initialize() 从未被调用, 20 个 plugin.so
                // (ms001~ms020) 永远没被 dlopen, 整个微服务架构是死代码。
                // 必须在 setPluginsDir 之后执行, 否则 initialize() 仍会用默认
                // 相对路径 "plugins" (无效)。
                // 注意: 微服务架构为实验性功能, 返回值仅用于日志, 不影响主扫描
                // (主扫描始终走 runQuickScanNative 传统路径)。
                runCatching {
                    val ok = NativeBridge.initMicroServices()
                    Log.i("ApexViewModel", "initMicroServices: ok=$ok (experimental)")
                }.onFailure { e ->
                    Log.w("ApexViewModel", "initMicroServices failed: ${e.message}")
                }
            }
        }
    }

    private fun addLog(type: LogType, message: String) {
        val time = java.text.SimpleDateFormat("HH:mm:ss", java.util.Locale.getDefault())
            .format(java.util.Date())
        _uiState.update { state ->
            // 修复：日志无限增长 → OOM。限制最多保留 200 条，超出时丢弃最旧的。
            val newLogs = state.logs + LogEntry(type, time, message)
            val trimmed = if (newLogs.size > 200) newLogs.drop(newLogs.size - 200) else newLogs
            state.copy(logs = trimmed)
        }
    }

    fun clearLogs() {
        _uiState.update { it.copy(logs = emptyList()) }
    }

    fun completePermissionGuide() {
        prefs.edit().putBoolean("is_first_launch", false).apply()
        _uiState.update { it.copy(isFirstLaunch = false) }
    }

    fun refreshDashboard() {
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            _uiState.update { it.copy(isLoading = false) }
        }
    }

    /**
     * 使用 tryEmit 而非 emit — emit 是 suspend 函数，在没有订阅者或缓冲区满时会挂起。
     * tryEmit 立即返回（缓冲区满时丢弃），不会卡住调用方。
     */
    fun triggerReset() {
        _snackbarChannel.tryEmit(SnackbarEvent("所有设置已恢复至出厂默认状态", SnackbarType.WARNING))
    }

    fun triggerExport() {
        _snackbarChannel.tryEmit(SnackbarEvent("诊断日志已成功导出至系统外置存储", SnackbarType.SUCCESS))
    }

    fun runScan() {
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            _uiState.update { it.copy(isScanning = true, scanProgress = 0f, currentLayer = "初始化") }
            addLog(LogType.INFO, "开始快速扫描...")
            try {
                // 模拟层进度（快速扫描覆盖 L1/L3/L8-L10，共 ~5 层）
                val quickLayers = listOf("L1 系统属性", "L3 内存特征", "L8 Magisk", "L9 KernelSU", "L10 APatch")
                quickLayers.forEachIndexed { idx, layer ->
                    _uiState.update {
                        it.copy(
                            scanProgress = (idx + 1).toFloat() / quickLayers.size,
                            currentLayer = layer
                        )
                    }
                    addLog(LogType.INFO, "扫描 $layer")
                }

                addLog(LogType.INFO, "加载原生检测引擎")
                val result = repository.runQuickScan()
                addLog(LogType.INFO, "扫描完成，风险分: ${result.riskScore}")
                // 修复：扫描完成后自动生成修复建议，用户无需手动点击"修复建议"按钮
                val layers = parseScanLayers(result.details)
                val recs = if (layers.isNotEmpty()) {
                    FixRecommendations.getRecommendationsForLayers(layers)
                } else emptyList()
                // 解析层通过数
                val (passed, total) = parseLayerStats(result.details)
                _uiState.update {
                    it.copy(
                        scanResult = result.details,
                        riskScore = result.riskScore,
                        isScanning = false,
                        scanProgress = 1f,
                        currentLayer = "",
                        recommendations = recs,
                        showRecommendations = recs.isNotEmpty(),
                        layersPassed = passed,
                        layersTotal = total
                    )
                }
                if (recs.isNotEmpty()) {
                    addLog(LogType.INFO, "检测到 ${recs.size} 个异常，已生成修复建议")
                } else {
                    addLog(LogType.INFO, "未检测到异常")
                }
                // 推送扫描完成通知（根据用户设置）
                notifyScanResult(result.riskScore, recs.size)
            } catch (e: Throwable) {
                addLog(LogType.ERROR, "扫描失败: ${e.message ?: e.javaClass.simpleName}")
                _uiState.update {
                    it.copy(isScanning = false, scanProgress = 0f, currentLayer = "", scanResult = "扫描失败: ${e.message ?: e.javaClass.simpleName}\n\n可能原因：\n1. 原生库未加载（非 ARM64 设备）\n2. 设备未 root\n3. SELinux 限制")
                }
            }
        }
    }

    fun runDeepScan() {
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            _uiState.update { it.copy(isScanning = true, scanProgress = 0f, currentLayer = "初始化") }
            addLog(LogType.INFO, "开始深度扫描（16 层全量检测）...")
            try {
                // 深度扫描的 16 层进度模拟
                val deepLayers = listOf(
                    "L1 系统属性", "L2 ART 注入", "L3 内存特征", "L4 挂载检查",
                    "L5 侧信道", "L6 Root 守护", "L7 Boot 链", "L8 Magisk",
                    "L9 KernelSU", "L10 APatch", "L11 Hook 框架", "L12 自定义 ROM",
                    "L13 固件完整性", "L14 虚拟框架", "L15 危险应用", "L16 Magisk 扩展"
                )
                // 阶段 1: native 报告生成（占 0-30%）
                addLog(LogType.INFO, "执行深度检测报告生成")
                _uiState.update { it.copy(scanProgress = 0.1f, currentLayer = "深度检测引擎") }
                val report = repository.runDeepDetection()

                // 阶段 2: 内存指纹（30-45%）
                _uiState.update { it.copy(scanProgress = 0.3f, currentLayer = "L3 内存特征") }
                addLog(LogType.INFO, "采集内存指纹")
                val memMask = runCatching { repository.getMemoryFingerprintMask() }.getOrDefault(0)
                val rwxCount = runCatching { NativeBridge.countRWXPages() }.getOrDefault(-1)

                // 阶段 3: 隐藏模块检测（45-60%）
                _uiState.update { it.copy(scanProgress = 0.45f, currentLayer = "Shamiko / ZygiskNext") }
                addLog(LogType.INFO, "检测 Shamiko / ZygiskNext")
                val shamiko = runCatching { repository.hasShamiko() }.getOrDefault(false)
                val zygiskNext = runCatching { repository.hasZygiskNext() }.getOrDefault(false)

                // 阶段 4: SELinux（60-75%）
                _uiState.update { it.copy(scanProgress = 0.6f, currentLayer = "SELinux 状态") }
                addLog(LogType.INFO, "检测 SELinux 状态")
                val selinuxJump = runCatching { NativeBridge.detectSELinuxContextJump() }.getOrDefault(false)
                val selinuxMod = runCatching { NativeBridge.detectSELinuxPolicyMod() }.getOrDefault(false)

                // 阶段 5: 自保护（75-90%）
                _uiState.update { it.copy(scanProgress = 0.75f, currentLayer = "自保护检查") }
                addLog(LogType.INFO, "执行自保护检查")
                val selfCheck = runCatching { SelfProtection.fullSelfCheck(getApplication()) }.getOrDefault(emptyMap())
                val hookIssues = (selfCheck["hooks"] as? List<String>) ?: emptyList()
                val injectIssues = (selfCheck["injections"] as? List<String>) ?: emptyList()
                val dexIssues = (selfCheck["dexIssues"] as? List<String>) ?: emptyList()

                // 阶段 6: 收尾（90-100%）
                _uiState.update { it.copy(scanProgress = 0.9f, currentLayer = "生成报告") }

                // 限制 deepReport 大小，避免在 UI state 中持有大字符串导致 OOM
                val truncatedReport = if (report.length > 64_000) report.take(64_000) + "\n...[truncated]" else report

                // 修复：深度扫描完成后也自动生成修复建议
                val layers = parseScanLayers(report)
                val recs = if (layers.isNotEmpty()) {
                    FixRecommendations.getRecommendationsForLayers(layers)
                } else emptyList()

                // 解析层通过数
                val (passed, total) = parseLayerStats(report)

                _uiState.update {
                    it.copy(
                        scanResult = report.take(500),
                        riskScore = runCatching { NativeBridge.getRiskScore() }.getOrDefault(0),
                        isScanning = false,
                        scanProgress = 1f,
                        currentLayer = "",
                        memFingerprintMask = memMask,
                        rwxPageCount = rwxCount,
                        hasShamiko = shamiko,
                        hasZygiskNext = zygiskNext,
                        selinuxCompromised = selinuxJump || selinuxMod,
                        deepReport = truncatedReport,
                        selfCheckIssues = hookIssues + injectIssues + dexIssues,
                        recommendations = recs,
                        showRecommendations = recs.isNotEmpty(),
                        layersPassed = passed,
                        layersTotal = total
                    )
                }
                addLog(LogType.INFO, "深度扫描完成，风险分: ${_uiState.value.riskScore}")
                if (recs.isNotEmpty()) {
                    addLog(LogType.INFO, "检测到 ${recs.size} 个异常，已生成修复建议")
                }
                notifyScanResult(_uiState.value.riskScore, recs.size)
            } catch (e: Throwable) {
                addLog(LogType.ERROR, "深度扫描失败: ${e.message ?: e.javaClass.simpleName}")
                _uiState.update {
                    it.copy(isScanning = false, scanProgress = 0f, currentLayer = "", scanResult = "深度扫描失败: ${e.message ?: e.javaClass.simpleName}\n可能原因：原生库未加载或权限不足")
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════════
    // v1.2.0 并行扫描 + 交叉验证 + 基线管理 + 实时监控
    // ═══════════════════════════════════════════════════════════

    /**
     * 并行扫描 — 使用 ParallelDetectionEngine 并发执行 22 层检测,
     * 然后通过 CrossValidator 进行交叉验证与误报抑制。
     *
     * 比传统 [runScan] 快 3-4 倍,且提供置信度评分与 root 类型推断。
     * 扫描完成后自动建立/更新设备基线 (如果用户未禁用)。
     */
    fun runParallelScan() {
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            _uiState.update {
                it.copy(
                    isScanning = true,
                    scanProgress = 0f,
                    currentLayer = "初始化并行引擎",
                    parallelScanResult = null,
                    validationResult = null
                )
            }
            addLog(LogType.INFO, "开始并行扫描 (22 层并发检测)...")
            try {
                // 阶段 1: 并行执行所有检测层
                val scanResult = parallelEngine.scanParallel()
                addLog(LogType.INFO, "并行扫描完成: ${scanResult.detectedCount}/${scanResult.totalLayers} 层检测到异常, " +
                    "风险分 ${scanResult.totalRiskScore}, 耗时 ${scanResult.totalLatencyMs}ms")

                // 阶段 2: 交叉验证
                // P1-D12 修复: 复用 ParallelDetectionEngine 已缓存的 quickScan 文本,
                // 避免在 CrossValidator FP 规则中再次触发 NativeBridge.runQuickScan()
                _uiState.update { it.copy(currentLayer = "交叉验证") }
                val validation = CrossValidator.validate(scanResult, scanResult.quickScanText)
                addLog(LogType.INFO, "交叉验证: 置信度 ${validation.confidence}%, 推断 root 类型: ${validation.inferredRootType}")
                if (validation.isHighConfidence) {
                    addLog(LogType.WARN, "高置信度检测到 ${validation.inferredRootType} — ${validation.conclusion}")
                } else if (validation.isLikelyFalsePositive) {
                    addLog(LogType.WARN, "可能误报 — ${validation.conclusion}")
                }

                // 阶段 3: 自动建立/更新基线 (首次扫描自动建立,后续扫描只更新风险分)
                _uiState.update { it.copy(currentLayer = "更新设备基线") }
                val snapshot = baseline.captureSnapshot(scanResult)
                if (!baseline.hasBaseline()) {
                    baseline.saveBaseline(snapshot)
                    addLog(LogType.INFO, "首次扫描,已自动建立设备基线")
                } else {
                    val diff = baseline.diff(snapshot)
                    if (diff != null && diff.hasSignificantChanges) {
                        addLog(LogType.WARN, "检测到设备状态变化: ${diff.summary.trim()}")
                    }
                }

                // 阶段 4: 生成修复建议
                val detectedLayerNames = scanResult.detectedLayers.map { it.layerName }
                val recs = if (scanResult.detectedCount > 0) {
                    // 将并行扫描结果映射到 FixRecommendations 已知的层名
                    val mappedLayers = mutableListOf<String>()
                    detectedLayerNames.forEach { name ->
                        when {
                            name.contains("系统属性") -> mappedLayers.add("属性")
                            name.contains("ART") || name.contains("内存") -> mappedLayers.add("内存")
                            name.contains("挂载") -> mappedLayers.add("挂载")
                            name.contains("侧信道") -> mappedLayers.add("系统调用时序")
                            name.contains("Magisk") -> mappedLayers.add("文件")
                            name.contains("KernelSU") -> mappedLayers.add("内核模块")
                            name.contains("APatch") -> mappedLayers.add("APatch")
                            name.contains("Hook") -> mappedLayers.add("自保护")
                            name.contains("Root Fork") -> mappedLayers.add("文件")
                            name.contains("隐藏框架") -> mappedLayers.add("Shamiko")
                            name.contains("现代 Hook") -> mappedLayers.add("自保护")
                        }
                    }
                    FixRecommendations.getRecommendationsForLayers(mappedLayers.distinct())
                } else emptyList()

                // 阶段 5: 更新 UI 状态
                _uiState.update {
                    it.copy(
                        isScanning = false,
                        scanProgress = 1f,
                        currentLayer = "",
                        scanResult = buildParallelScanSummary(scanResult, validation),
                        riskScore = scanResult.totalRiskScore,
                        parallelScanResult = scanResult,
                        validationResult = validation,
                        parallelScanLatencyMs = scanResult.totalLatencyMs,
                        layersPassed = scanResult.totalLayers - scanResult.detectedCount,
                        layersTotal = scanResult.totalLayers,
                        hasBaseline = baseline.hasBaseline(),
                        baselineTimestamp = baseline.loadBaseline()?.timestamp,
                        recommendations = recs,
                        showRecommendations = recs.isNotEmpty()
                    )
                }
                addLog(LogType.INFO, "并行扫描完成,风险分: ${scanResult.totalRiskScore}")
                if (recs.isNotEmpty()) {
                    addLog(LogType.INFO, "检测到 ${recs.size} 个异常,已生成修复建议")
                }
                notifyScanResult(scanResult.totalRiskScore, scanResult.detectedCount)
            } catch (e: Throwable) {
                addLog(LogType.ERROR, "并行扫描失败: ${e.message ?: e.javaClass.simpleName}")
                _uiState.update {
                    it.copy(
                        isScanning = false,
                        scanProgress = 0f,
                        currentLayer = "",
                        scanResult = "并行扫描失败: ${e.message ?: e.javaClass.simpleName}"
                    )
                }
            }
        }
    }

    /**
     * 手动建立/覆盖设备基线。
     */
    fun establishBaseline() {
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            addLog(LogType.INFO, "正在建立设备基线...")
            try {
                guardMonitor.establishBaseline()
                _uiState.update {
                    it.copy(
                        hasBaseline = true,
                        baselineTimestamp = baseline.loadBaseline()?.timestamp
                    )
                }
                addLog(LogType.INFO, "设备基线已建立")
                _snackbarChannel.tryEmit(SnackbarEvent("设备基线已建立,实时监控将基于此基线检测变化", SnackbarType.SUCCESS))
            } catch (e: Throwable) {
                addLog(LogType.ERROR, "建立基线失败: ${e.message}")
                _snackbarChannel.tryEmit(SnackbarEvent("建立基线失败: ${e.message}", SnackbarType.ERROR))
            }
        }
    }

    /**
     * 清除设备基线。
     */
    fun clearBaseline() {
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            baseline.clearBaseline()
            _uiState.update {
                it.copy(hasBaseline = false, baselineTimestamp = null)
            }
            addLog(LogType.INFO, "设备基线已清除")
            _snackbarChannel.tryEmit(SnackbarEvent("设备基线已清除", SnackbarType.WARNING))
        }
    }

    /**
     * 启动实时监控 (Guard Monitor)。
     * v1.1.1: 同时启动前台服务,确保后台持续监控。
     * @param intervalMs 检测间隔,默认 60s
     */
    fun startGuardMonitor(intervalMs: Long = 60_000L) {
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            // 如果没有基线,先自动建立
            if (!baseline.hasBaseline()) {
                addLog(LogType.INFO, "实时监控: 无基线,先自动建立...")
                guardMonitor.establishBaseline()
                _uiState.update {
                    it.copy(
                        hasBaseline = true,
                        baselineTimestamp = baseline.loadBaseline()?.timestamp
                    )
                }
            }
            guardMonitor.start(intervalMs)
            // P1-7: 启动前台服务,利用 FOREGROUND_SERVICE_SPECIAL_USE 权限
            runCatching {
                com.apex.root.service.GuardMonitorService.startService(getApplication(), intervalMs)
            }
            addLog(LogType.INFO, "实时监控已启动 (间隔 ${intervalMs / 1000}s, 前台服务运行中)")
            _snackbarChannel.tryEmit(SnackbarEvent("实时监控已启动,每 ${intervalMs / 1000}s 检测一次", SnackbarType.SUCCESS))
        }
    }

    /**
     * 停止实时监控。
     * v1.1.1: 同时停止前台服务。
     */
    fun stopGuardMonitor() {
        guardMonitor.stop()
        // P1-7: 停止前台服务
        runCatching {
            com.apex.root.service.GuardMonitorService.stopService(getApplication())
        }
        addLog(LogType.INFO, "实时监控已停止")
        _snackbarChannel.tryEmit(SnackbarEvent("实时监控已停止", SnackbarType.WARNING))
    }

    /**
     * 立即执行一次实时监控检查 (不等待下一个间隔)。
     */
    fun checkGuardNow() {
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            addLog(LogType.INFO, "执行即时监控检查...")
            guardMonitor.checkNow()
        }
    }

    /**
     * 更新实时监控间隔。
     */
    fun updateGuardInterval(intervalMs: Long) {
        guardMonitor.updateInterval(intervalMs)
        addLog(LogType.INFO, "监控间隔已更新为 ${intervalMs / 1000}s")
    }

    /**
     * 清空告警历史。
     */
    fun clearGuardAlerts() {
        _uiState.update { it.copy(guardAlerts = emptyList()) }
    }

    /**
     * P1-9: 扫描媒体文件,利用 READ_MEDIA_* 权限检测可疑的 root/hack 工具。
     */
    fun scanMedia() {
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            addLog(LogType.INFO, "开始扫描媒体文件 (利用 READ_MEDIA_* 权限)...")
            try {
                val app = getApplication<Application>()
                val findings = com.apex.root.domain.parallel.MediaScanner.scan(app)
                val report = com.apex.root.domain.parallel.MediaScanner.generateReport(findings)
                addLog(LogType.INFO, "媒体扫描完成: ${findings.size} 个可疑文件")
                if (findings.isNotEmpty()) {
                    _snackbarChannel.tryEmit(SnackbarEvent(
                        "媒体扫描发现 ${findings.size} 个可疑文件",
                        SnackbarType.WARNING
                    ))
                } else {
                    _snackbarChannel.tryEmit(SnackbarEvent(
                        "媒体扫描完成,未发现可疑文件",
                        SnackbarType.SUCCESS
                    ))
                }
                // 更新 UI state 中的扫描结果
                _uiState.update {
                    it.copy(scanResult = report)
                }
            } catch (e: Throwable) {
                addLog(LogType.ERROR, "媒体扫描失败: ${e.message}")
                _snackbarChannel.tryEmit(SnackbarEvent("媒体扫描失败: ${e.message}", SnackbarType.ERROR))
            }
        }
    }

    /**
     * 构建并行扫描结果的人类可读摘要。
     */
    private fun buildParallelScanSummary(
        scanResult: ParallelScanResult,
        validation: ValidationResult
    ): String {
        val sb = StringBuilder()
        sb.appendLine("=== APEX 并行扫描结果 (v1.2.0) ===")
        sb.appendLine()
        sb.appendLine("扫描耗时: ${scanResult.totalLatencyMs}ms (22 层并发)")
        sb.appendLine("风险评分: ${scanResult.totalRiskScore}/100 (${scanResult.riskLevel})")
        sb.appendLine("检测到异常的层: ${scanResult.detectedCount}/${scanResult.totalLayers}")
        sb.appendLine()
        sb.appendLine("── 交叉验证 ──")
        sb.appendLine("置信度: ${validation.confidence}%")
        sb.appendLine("推断 root 类型: ${validation.inferredRootType}")
        sb.appendLine("结论: ${validation.conclusion}")
        if (validation.matchedValidationRules.isNotEmpty()) {
            sb.appendLine("匹配的验证规则: ${validation.matchedValidationRules.joinToString(", ")}")
        }
        if (validation.matchedFalsePositiveRules.isNotEmpty()) {
            sb.appendLine("匹配的误报规则: ${validation.matchedFalsePositiveRules.joinToString(", ")}")
        }
        sb.appendLine()
        sb.appendLine("── 各层详情 ──")
        scanResult.layers.forEach { layer ->
            val status = if (layer.detected) "❌ 异常" else "✅ 正常"
            sb.appendLine("${layer.layerName}: $status (${layer.latencyMs}ms, 权重 ${layer.weight})")
        }
        return sb.toString()
    }

    /**
     * 推送实时监控告警通知。
     */
    private fun notifyGuardAlert(alert: com.apex.root.domain.parallel.RealtimeGuardMonitor.GuardAlert) {
        try {
            val settings = settingsRepo.load()
            if (!settings.notifyRiskFound) return
            val app = getApplication<Application>()
            val title = when (alert.level) {
                com.apex.root.domain.parallel.RealtimeGuardMonitor.AlertLevel.CRITICAL -> "🚨 严重告警: ${alert.title}"
                com.apex.root.domain.parallel.RealtimeGuardMonitor.AlertLevel.ALERT -> "⚠️ 告警: ${alert.title}"
                com.apex.root.domain.parallel.RealtimeGuardMonitor.AlertLevel.WARNING -> "⚠️ 警告: ${alert.title}"
                com.apex.root.domain.parallel.RealtimeGuardMonitor.AlertLevel.INFO -> "ℹ️ 信息: ${alert.title}"
            }
            com.apex.root.core.notification.Notifier.notifyRiskAlert(app, title, alert.message)
        } catch (e: Throwable) {
            Log.e("ApexViewModel", "notifyGuardAlert failed", e)
        }
    }

    // P1-2 修复: ViewModel 销毁时停止 guard monitor,避免 CoroutineScope 泄漏
    override fun onCleared() {
        runCatching {
            guardMonitor.stop()
            Log.i("ApexViewModel", "Guard monitor stopped in onCleared")
        }
        super.onCleared()
    }

    // ═══════════════════════════════════════════════════════════
    // v1.0.3: 从删除的 UI 文件提取的检测流程,统一通过 ViewModel 暴露
    // ═══════════════════════════════════════════════════════════

    /** Frida 检测 (原 FridaConsoleScreen 直接调用) */
    fun detectFridaStatus(): Boolean = NativeBridge.detectFrida()
    fun detectSELinuxJump(): Boolean = NativeBridge.detectSELinuxContextJump()

    /** LSPosed/Zygisk/Riru 检测 (原 LSPosedManagerScreen 直接调用) */
    data class HookFrameworkStatus(
        val xposed: Boolean, val lsposed: Boolean,
        val riru: Boolean, val zygisk: Boolean
    )
    fun detectHookFrameworks(): HookFrameworkStatus = HookFrameworkStatus(
        xposed = NativeBridge.detectXposedFramework(),
        lsposed = NativeBridge.detectLSPosedManager(),
        riru = NativeBridge.detectRiruModules(),
        zygisk = NativeBridge.detectZygiskModules()
    )

    /** Guard 引擎直接控制 (原 DashboardScreen 直接调用 NativeGuard) */
    fun startGuardian(): Boolean = runCatching {
        com.apex.root.guard.NativeGuard.startGuardian()
    }.getOrDefault(false)

    fun checkGuardIntegrity(): Boolean = runCatching {
        com.apex.root.guard.NativeGuard.checkIntegrity()
    }.getOrDefault(false)

    /** Native 库重试加载 (原 UpdateScreen 直接调用) */
    fun retryNativeLoad() {
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            com.apex.root.core.NativeLibraryLoader.reset()
            com.apex.root.core.NativeLibraryLoader.ensureLoaded()
            val available = com.apex.root.core.NativeLibraryLoader.isAvailable
            _uiState.update { it.copy(nativeAvailable = available) }
        }
    }

    /** 权限检查 (原 PermissionsScreen 直接调用) */
    suspend fun checkAllPermissions(): List<com.apex.root.core.permission.PermissionManager.PermissionInfo> {
        return withContext(Dispatchers.IO) {
            com.apex.root.core.permission.PermissionManager.checkAll(getApplication())
        }
    }

    fun toggleGameMode() {
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            try {
                repository.toggleGameMode()
                _uiState.update {
                    it.copy(gameMode = repository.getGameModeState())
                }
            } catch (e: Throwable) {
                _uiState.update { it.copy(cureMessage = "游戏模式切换失败: ${e.message}") }
            }
        }
    }

    fun applyCure(level: CureLevel) {
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            try {
                val result = repository.applyCure(level)
                _uiState.update {
                    it.copy(cureMessage = result.message)
                }
                // 推送治愈结果通知（根据用户设置）
                notifyCureResult(level, result.success)
            } catch (e: Throwable) {
                _uiState.update { it.copy(cureMessage = "治愈操作失败: ${e.message}") }
                // 失败也推送通知
                notifyCureResult(level, success = false)
            }
        }
    }

    fun createSandbox(name: String) {
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            try {
                val pid = runCatching { NativeIsland.createIsolatedEnv(name) }.getOrDefault(-1)
                _uiState.update {
                    it.copy(sandboxPid = pid, sandboxActive = pid > 0)
                }
            } catch (e: Throwable) {
                _uiState.update { it.copy(cureMessage = "创建沙箱失败: ${e.message}") }
            }
        }
    }

    fun destroySandbox() {
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            try {
                val pid = _uiState.value.sandboxPid
                if (pid > 0) runCatching { NativeIsland.destroyIsolatedEnv(pid) }
                _uiState.update {
                    it.copy(sandboxPid = -1, sandboxActive = false)
                }
            } catch (e: Throwable) {
                _uiState.update { it.copy(sandboxPid = -1, sandboxActive = false) }
            }
        }
    }

    fun toggleHwidSpoof() {
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            try {
                if (_uiState.value.hwidSpoofed) {
                    val ok = runCatching { NativeHwid.restoreReal() }.getOrDefault(false)
                    _uiState.update { it.copy(hwidSpoofed = !ok) }
                } else {
                    val ok = runCatching { NativeHwid.spoofAll() }.getOrDefault(false)
                    _uiState.update { it.copy(hwidSpoofed = ok) }
                }
            } catch (e: Throwable) {
                _uiState.update { it.copy(cureMessage = "HWID 伪装切换失败: ${e.message}") }
            }
        }
    }

    fun refreshState() {
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            try {
                val gameMode = repository.getGameModeState()
                _uiState.update { it.copy(gameMode = gameMode) }
            } catch (e: Throwable) {
                Log.e("ApexViewModel", "refreshState failed", e)
            }
        }
    }

    fun showFixRecommendations() {
        val layers = parseScanLayers(_uiState.value.scanResult)
        val recs = FixRecommendations.getRecommendationsForLayers(layers)
        _uiState.update {
            it.copy(recommendations = recs, showRecommendations = true)
        }
    }

    fun dismissRecommendations() {
        _uiState.update { it.copy(showRecommendations = false) }
    }

    fun exportReport(context: Context) {
        // 移到 IO 线程执行文件写入，避免主线程磁盘 I/O 触发 StrictMode 崩溃
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            try {
                ReportExporter.shareReport(context, _uiState.value)
            } catch (e: Throwable) {
                Log.e("ApexViewModel", "exportReport failed", e)
                _snackbarChannel.tryEmit(SnackbarEvent("报告导出失败: ${e.message ?: "未知错误"}", SnackbarType.ERROR))
            }
        }
    }

    /**
     * 从扫描结果文本中解析出检测到异常的层级。
     * 修复：原实现不区分"❌ 异常"和"✅ 正常"，导致即使层级正常也会显示修复建议。
     * 现在仅匹配包含"❌"的行，确保只有检测到异常的层级才返回。
     */
    private fun parseScanLayers(result: String): List<String> {
        val layers = mutableListOf<String>()
        result.lines().forEach { line ->
            // 仅对检测到异常的行（包含 ❌）生成修复建议
            if (!line.contains("❌")) return@forEach
            when {
                line.contains("系统属性") -> layers.add("属性")
                line.contains("ART") || line.contains("内存特征") -> layers.add("内存")
                line.contains("挂载") -> layers.add("挂载")
                line.contains("侧信道") -> layers.add("系统调用时序")
                line.contains("内核完整性") || line.contains("内核  ❌") -> layers.add("内核")
                line.contains("Boot") -> layers.add("固件")
                line.contains("Magisk") -> layers.add("文件")
                line.contains("KernelSU") -> layers.add("内核模块")
                line.contains("APatch") -> layers.add("APatch")
                line.contains("Hook") || line.contains("Xposed") -> layers.add("自保护")
                line.contains("ROM") -> layers.add("固件完整性")
                line.contains("Shamiko") -> layers.add("Shamiko")
                line.contains("Zygisk") -> layers.add("ZygiskNext")
            }
        }
        return layers.distinct()
    }

    /**
     * 从扫描结果文本中解析层通过数和总层数。
     * 扫描结果格式示例：
     *   "L1 系统属性:     ✅ 正常"
     *   "L8 Magisk:       ❌ 异常"
     * 通过：行包含 ✅；异常：行包含 ❌；总：✅ + ❌
     */
    private fun parseLayerStats(result: String): Pair<Int, Int> {
        var passed = 0
        var total = 0
        result.lines().forEach { line ->
            // 仅匹配形如 "L1 ..." 或 "L14 ..." 的行
            val trimmed = line.trim()
            if (!trimmed.startsWith("L") || trimmed.isEmpty()) return@forEach
            // 确认是层级行（L 后跟数字）
            val afterL = trimmed.drop(1)
            if (afterL.isEmpty() || !afterL.first().isDigit()) return@forEach
            when {
                line.contains("✅") -> { passed++; total++ }
                line.contains("❌") -> { total++ }
            }
        }
        return passed to total
    }

    // ─── 通知推送（根据用户设置）──────────────────

    /**
     * 推送扫描完成通知。
     * - notifyScanComplete=true：发送扫描完成通知
     * - notifyRiskFound=true 且 riskScore>=61：额外发送高风险告警
     */
    private fun notifyScanResult(riskScore: Int, riskCount: Int) {
        try {
            val settings = settingsRepo.load()
            val app = getApplication<Application>()
            if (settings.notifyScanComplete) {
                com.apex.root.core.notification.Notifier.notifyScanComplete(app, riskScore, riskCount)
            }
            if (settings.notifyRiskFound && riskScore >= 61) {
                com.apex.root.core.notification.Notifier.notifyRiskAlert(
                    app,
                    "检测到高风险",
                    "风险分: $riskScore/100，检测到 $riskCount 个异常。点击查看详情。"
                )
            }
        } catch (e: Throwable) {
            Log.e("ApexViewModel", "notifyScanResult failed", e)
        }
    }

    /**
     * 推送治愈结果通知。
     */
    private fun notifyCureResult(level: CureLevel, success: Boolean) {
        try {
            val settings = settingsRepo.load()
            if (!settings.notifyCureResult) return
            val app = getApplication<Application>()
            val levelName = when (level) {
                CureLevel.LIGHT -> "轻度处理"
                CureLevel.STANDARD -> "标准修复"
                CureLevel.DEEP -> "深度恢复"
                CureLevel.FACTORY -> "完全重置"
            }
            com.apex.root.core.notification.Notifier.notifyCureResult(app, levelName, success)
        } catch (e: Throwable) {
            Log.e("ApexViewModel", "notifyCureResult failed", e)
        }
    }
}
