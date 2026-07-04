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
    val showRecommendations: Boolean = false
)

class ApexViewModel(application: Application) : AndroidViewModel(application) {
    private val repository = RootDetectRepositoryImpl()
    private val prefs = application.getSharedPreferences("apex_prefs", Context.MODE_PRIVATE)
    private val _uiState = MutableStateFlow(ApexUiState())
    val uiState: StateFlow<ApexUiState> = _uiState.asStateFlow()

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
    }

    private fun checkNativeAvailability() {
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            val available = runCatching {
                com.apex.root.core.NativeLibraryLoader.ensureLoaded()
                com.apex.root.core.NativeLibraryLoader.isAvailable
            }.getOrDefault(false)
            _uiState.update { it.copy(nativeAvailable = available) }
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
            _uiState.update { it.copy(isScanning = true) }
            addLog(LogType.INFO, "开始快速扫描...")
            try {
                addLog(LogType.INFO, "加载原生检测引擎")
                val result = repository.runQuickScan()
                addLog(LogType.INFO, "扫描完成，风险分: ${result.riskScore}")
                // 修复：扫描完成后自动生成修复建议，用户无需手动点击"修复建议"按钮
                val layers = parseScanLayers(result.details)
                val recs = if (layers.isNotEmpty()) {
                    FixRecommendations.getRecommendationsForLayers(layers)
                } else emptyList()
                _uiState.update {
                    it.copy(
                        scanResult = result.details,
                        riskScore = result.riskScore,
                        isScanning = false,
                        recommendations = recs,
                        showRecommendations = recs.isNotEmpty()
                    )
                }
                if (recs.isNotEmpty()) {
                    addLog(LogType.INFO, "检测到 ${recs.size} 个异常，已生成修复建议")
                } else {
                    addLog(LogType.INFO, "未检测到异常")
                }
            } catch (e: Throwable) {
                addLog(LogType.ERROR, "扫描失败: ${e.message ?: e.javaClass.simpleName}")
                _uiState.update {
                    it.copy(isScanning = false, scanResult = "扫描失败: ${e.message ?: e.javaClass.simpleName}\n\n可能原因：\n1. 原生库未加载（非 ARM64 设备）\n2. 设备未 root\n3. SELinux 限制")
                }
            }
        }
    }

    fun runDeepScan() {
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            _uiState.update { it.copy(isScanning = true) }
            addLog(LogType.INFO, "开始深度扫描（16 层全量检测）...")
            try {
                addLog(LogType.INFO, "执行深度检测报告生成")
                val report = repository.runDeepDetection()
                addLog(LogType.INFO, "采集内存指纹")
                val memMask = runCatching { repository.getMemoryFingerprintMask() }.getOrDefault(0)
                val rwxCount = runCatching { NativeBridge.countRWXPages() }.getOrDefault(-1)
                addLog(LogType.INFO, "检测 Shamiko / ZygiskNext")
                val shamiko = runCatching { repository.hasShamiko() }.getOrDefault(false)
                val zygiskNext = runCatching { repository.hasZygiskNext() }.getOrDefault(false)
                addLog(LogType.INFO, "检测 SELinux 状态")
                val selinuxJump = runCatching { NativeBridge.detectSELinuxContextJump() }.getOrDefault(false)
                val selinuxMod = runCatching { NativeBridge.detectSELinuxPolicyMod() }.getOrDefault(false)
                addLog(LogType.INFO, "执行自保护检查")
                val selfCheck = runCatching { SelfProtection.fullSelfCheck(getApplication()) }.getOrDefault(emptyMap())
                val hookIssues = (selfCheck["hooks"] as? List<String>) ?: emptyList()
                val injectIssues = (selfCheck["injections"] as? List<String>) ?: emptyList()
                val dexIssues = (selfCheck["dexIssues"] as? List<String>) ?: emptyList()

                // 限制 deepReport 大小，避免在 UI state 中持有大字符串导致 OOM
                val truncatedReport = if (report.length > 64_000) report.take(64_000) + "\n...[truncated]" else report

                // 修复：深度扫描完成后也自动生成修复建议
                val layers = parseScanLayers(report)
                val recs = if (layers.isNotEmpty()) {
                    FixRecommendations.getRecommendationsForLayers(layers)
                } else emptyList()

                _uiState.update {
                    it.copy(
                        scanResult = report.take(500),
                        riskScore = runCatching { NativeBridge.getRiskScore() }.getOrDefault(0),
                        isScanning = false,
                        memFingerprintMask = memMask,
                        rwxPageCount = rwxCount,
                        hasShamiko = shamiko,
                        hasZygiskNext = zygiskNext,
                        selinuxCompromised = selinuxJump || selinuxMod,
                        deepReport = truncatedReport,
                        selfCheckIssues = hookIssues + injectIssues + dexIssues,
                        recommendations = recs,
                        showRecommendations = recs.isNotEmpty()
                    )
                }
                addLog(LogType.INFO, "深度扫描完成，风险分: ${_uiState.value.riskScore}")
                if (recs.isNotEmpty()) {
                    addLog(LogType.INFO, "检测到 ${recs.size} 个异常，已生成修复建议")
                }
            } catch (e: Throwable) {
                addLog(LogType.ERROR, "深度扫描失败: ${e.message ?: e.javaClass.simpleName}")
                _uiState.update {
                    it.copy(isScanning = false, scanResult = "深度扫描失败: ${e.message ?: e.javaClass.simpleName}\n可能原因：原生库未加载或权限不足")
                }
            }
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
            } catch (e: Throwable) {
                _uiState.update { it.copy(cureMessage = "治愈操作失败: ${e.message}") }
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
}
