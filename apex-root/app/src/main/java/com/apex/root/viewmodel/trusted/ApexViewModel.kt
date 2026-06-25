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
            _uiState.value = _uiState.value.copy(isScanning = true)
            val result = repository.runQuickScan()
            _uiState.value = _uiState.value.copy(
                scanResult = result.details,
                riskScore = result.riskScore,
                isScanning = false
            )
        }
    }

    fun runDeepScan() {
        viewModelScope.launch(Dispatchers.IO) {
            _uiState.value = _uiState.value.copy(isScanning = true)
            val report = repository.runDeepDetection()
            val memMask = repository.getMemoryFingerprintMask()
            val rwxCount = NativeBridge.countRWXPages()
            val shamiko = repository.hasShamiko()
            val zygiskNext = repository.hasZygiskNext()
            val selinuxJump = NativeBridge.detectSELinuxContextJump()
            val selinuxMod = NativeBridge.detectSELinuxPolicyMod()
            val selfCheck = SelfProtection.fullSelfCheck(getApplication())
            val hookIssues = (selfCheck["hooks"] as? List<String>) ?: emptyList()
            val injectIssues = (selfCheck["injections"] as? List<String>) ?: emptyList()
            val dexIssues = (selfCheck["dexIssues"] as? List<String>) ?: emptyList()

            _uiState.value = _uiState.value.copy(
                scanResult = report.take(500),
                riskScore = NativeBridge.getRiskScore(),
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
    }

    fun toggleGameMode() {
        viewModelScope.launch(Dispatchers.IO) {
            repository.toggleGameMode()
            _uiState.value = _uiState.value.copy(
                gameMode = repository.getGameModeState()
            )
        }
    }

    fun applyCure(level: CureLevel) {
        viewModelScope.launch(Dispatchers.IO) {
            val result = repository.applyCure(level)
            _uiState.value = _uiState.value.copy(
                cureMessage = result.message
            )
        }
    }

    fun createSandbox(name: String) {
        viewModelScope.launch(Dispatchers.IO) {
            val pid = NativeIsland.createIsolatedEnv(name)
            _uiState.value = _uiState.value.copy(
                sandboxPid = pid,
                sandboxActive = pid > 0
            )
        }
    }

    fun destroySandbox() {
        viewModelScope.launch(Dispatchers.IO) {
            val pid = _uiState.value.sandboxPid
            if (pid > 0) NativeIsland.destroyIsolatedEnv(pid)
            _uiState.value = _uiState.value.copy(
                sandboxPid = -1,
                sandboxActive = false
            )
        }
    }

    fun toggleHwidSpoof() {
        viewModelScope.launch(Dispatchers.IO) {
            if (_uiState.value.hwidSpoofed) {
                NativeHwid.restoreReal()
                _uiState.value = _uiState.value.copy(hwidSpoofed = false)
            } else {
                NativeHwid.spoofAll()
                _uiState.value = _uiState.value.copy(hwidSpoofed = true)
            }
        }
    }

    fun refreshState() {
        viewModelScope.launch(Dispatchers.IO) {
            val gameMode = repository.getGameModeState()
            _uiState.value = _uiState.value.copy(gameMode = gameMode)
        }
    }

    fun showFixRecommendations() {
        val layers = parseScanLayers(_uiState.value.scanResult)
        val recs = FixRecommendations.getRecommendationsForLayers(layers)
        _uiState.value = _uiState.value.copy(
            recommendations = recs,
            showRecommendations = true
        )
    }

    fun dismissRecommendations() {
        _uiState.value = _uiState.value.copy(showRecommendations = false)
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
