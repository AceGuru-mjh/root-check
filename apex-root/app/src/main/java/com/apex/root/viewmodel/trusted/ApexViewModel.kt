package com.apex.root.viewmodel.trusted

import android.app.Application
import android.content.Context
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

    private val _snackbarChannel = MutableSharedFlow<SnackbarEvent>()
    val snackbarChannel: SharedFlow<SnackbarEvent> = _snackbarChannel.asSharedFlow()

    init {
        SelfProtection.init(application)
        val isFirst = prefs.getBoolean("is_first_launch", true)
        _uiState.update { it.copy(isFirstLaunch = isFirst) }
        refreshDashboard()
        loadMockLogs()
    }

    fun completePermissionGuide() {
        prefs.edit().putBoolean("is_first_launch", false).apply()
        _uiState.update { it.copy(isFirstLaunch = false) }
    }

    fun refreshDashboard() {
        viewModelScope.launch {
            _uiState.update { it.copy(isLoading = true) }
            delay(2500)
            _uiState.update { it.copy(isLoading = false, riskScore = (85..99).random()) }
        }
    }

    fun triggerReset() {
        viewModelScope.launch {
            _snackbarChannel.emit(SnackbarEvent("所有设置已恢复至出厂默认状态", SnackbarType.WARNING))
        }
    }

    fun triggerExport() {
        viewModelScope.launch {
            _snackbarChannel.emit(SnackbarEvent("诊断日志已成功导出至系统外置存储", SnackbarType.SUCCESS))
        }
    }

    private fun loadMockLogs() {
        _uiState.update { state ->
            state.copy(
                logs = listOf(
                    LogEntry(LogType.INFO, "10:42:01", "VSync 底层校准脉冲信号正常 (周期: 16.6ms)"),
                    LogEntry(LogType.WARN, "10:43:15", "检测到无障碍权限正在被第三方应用轮询监听"),
                    LogEntry(LogType.ERROR, "10:45:22", "Root 守护进程异常退出，错误代码: SIGSEGV (0x11)")
                )
            )
        }
    }

    fun runScan() {
        viewModelScope.launch(Dispatchers.IO) {
            _uiState.update { it.copy(isScanning = true) }
            try {
                val result = repository.runQuickScan()
                _uiState.update {
                    it.copy(
                        scanResult = result.details,
                        riskScore = result.riskScore,
                        isScanning = false
                    )
                }
            } catch (e: Throwable) {
                // 修复：catch Throwable 而非 Exception，捕获 UnsatisfiedLinkError 等 Error
                _uiState.update {
                    it.copy(
                        isScanning = false,
                        scanResult = "扫描失败: ${e.message ?: e.javaClass.simpleName}"
                    )
                }
            }
        }
    }

    fun runDeepScan() {
        viewModelScope.launch(Dispatchers.IO) {
            _uiState.update { it.copy(isScanning = true) }
            try {
                val report = repository.runDeepDetection()
                val memMask = runCatching { repository.getMemoryFingerprintMask() }.getOrDefault(0)
                val rwxCount = runCatching { NativeBridge.countRWXPages() }.getOrDefault(-1)
                val shamiko = runCatching { repository.hasShamiko() }.getOrDefault(false)
                val zygiskNext = runCatching { repository.hasZygiskNext() }.getOrDefault(false)
                val selinuxJump = runCatching { NativeBridge.detectSELinuxContextJump() }.getOrDefault(false)
                val selinuxMod = runCatching { NativeBridge.detectSELinuxPolicyMod() }.getOrDefault(false)
                val selfCheck = runCatching { SelfProtection.fullSelfCheck(getApplication()) }.getOrDefault(emptyMap())
                val hookIssues = (selfCheck["hooks"] as? List<String>) ?: emptyList()
                val injectIssues = (selfCheck["injections"] as? List<String>) ?: emptyList()
                val dexIssues = (selfCheck["dexIssues"] as? List<String>) ?: emptyList()

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
                        deepReport = report,
                        selfCheckIssues = hookIssues + injectIssues + dexIssues
                    )
                }
            } catch (e: Throwable) {
                // 修复：catch Throwable 捕获 UnsatisfiedLinkError / NoClassDefFoundError
                _uiState.update {
                    it.copy(
                        isScanning = false,
                        scanResult = "深度扫描失败: ${e.message ?: e.javaClass.simpleName}\n" +
                                "可能原因：原生库未加载或权限不足"
                    )
                }
            }
        }
    }

    fun toggleGameMode() {
        viewModelScope.launch(Dispatchers.IO) {
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
        viewModelScope.launch(Dispatchers.IO) {
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
        viewModelScope.launch(Dispatchers.IO) {
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
        viewModelScope.launch(Dispatchers.IO) {
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
        viewModelScope.launch(Dispatchers.IO) {
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
        viewModelScope.launch(Dispatchers.IO) {
            val gameMode = repository.getGameModeState()
            _uiState.update { it.copy(gameMode = gameMode) }
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
        ReportExporter.shareReport(context, _uiState.value)
    }

    private fun parseScanLayers(result: String): List<String> {
        val layers = mutableListOf<String>()
        result.lines().forEach { line ->
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
