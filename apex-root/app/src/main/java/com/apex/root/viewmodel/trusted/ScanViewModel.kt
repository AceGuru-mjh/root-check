package com.apex.root.viewmodel.trusted

import android.app.Application
import android.util.Log
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.apex.root.domain.trust.model.*
import com.apex.root.ipc.DetectionProtocol
import com.apex.root.ipc.client.SecureSocketClient
import kotlinx.coroutines.CoroutineExceptionHandler
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.*
import kotlinx.coroutines.launch
import java.io.ByteArrayOutputStream
import java.io.DataOutputStream
import java.util.UUID

sealed class UiState {
    data object Idle : UiState()
    data object Connecting : UiState()
    data object Scanning : UiState()
    data class Report(val report: GlobalSecureReport) : UiState()
    data class Alert(val alert: SecurityAlert) : UiState()
    data class Error(val message: String) : UiState()
}

class ScanViewModel(application: Application) : AndroidViewModel(application) {
    private val _uiState = MutableStateFlow<UiState>(UiState.Idle)
    val uiState: StateFlow<UiState> = _uiState.asStateFlow()

    private val _progress = MutableStateFlow(0f)
    val progress: StateFlow<Float> = _progress.asStateFlow()

    // P1-6 修复: 保存超时 Job,在扫描完成/ViewModel 销毁时取消,避免协程泄漏
    private var timeoutJob: kotlinx.coroutines.Job? = null

    /**
     * 警报历史列表（累积所有收到的警报，不丢失）。
     * 与 [uiState] 中的单个 Alert 状态互补：uiState 反映"当前活跃警报"，
     * alerts 反映"历史所有警报"。UI 可同时展示当前警报 + 历史列表。
     */
    private val _alerts = MutableStateFlow<List<SecurityAlert>>(emptyList())
    val alerts: StateFlow<List<SecurityAlert>> = _alerts.asStateFlow()

    /**
     * 全局协程异常处理器 — 任何未捕获异常记录但不崩溃进程。
     */
    private val exceptionHandler = CoroutineExceptionHandler { _, e ->
        Log.e("ScanViewModel", "Uncaught coroutine exception", e)
    }

    // 修复：SecureSocketClient 需要 scope 参数（原构造仅传 socketName 会导致编译失败）
    private val client = SecureSocketClient("apex_root_sandbox", viewModelScope)

    init {
        viewModelScope.launch(exceptionHandler) {
            try {
                client.messages.collect { data ->
                    handleMessage(data)
                }
            } catch (e: Throwable) {
                _uiState.value = UiState.Error("IPC 消息流异常: ${e.message ?: e.javaClass.simpleName}")
            }
        }
        viewModelScope.launch(exceptionHandler) {
            try {
                client.connectionState.collect { connected ->
                    // 仅在之前处于 Scanning 时报错，避免覆盖更具体的错误信息
                    if (!connected && _uiState.value is UiState.Scanning) {
                        _uiState.value = UiState.Error("IPC connection lost")
                    }
                }
            } catch (_: Throwable) {}
        }
    }

    fun connect() {
        _uiState.value = UiState.Connecting
        viewModelScope.launch(exceptionHandler) {
            val ok = try { client.connect() } catch (_: Throwable) { false }
            if (!ok) {
                _uiState.value = UiState.Error("Failed to connect to sandbox")
            } else {
                _uiState.value = UiState.Idle
            }
        }
    }

    fun startScan(level: DetectionLevel) {
        viewModelScope.launch(exceptionHandler) {
            _uiState.value = UiState.Scanning
            _progress.value = 0f

            val task = ScanTask(
                taskId = UUID.randomUUID().toString(),
                level = level,
                enabledServices = listOf("*"),
                // P0-S4 修复 (v1.1.1): 之前 nonce = ByteArray(32) { (it % 256).toByte() }
                // 生成 [0,1,2,...,31] 完全可预测, 重放攻击可绕过 nonce 防御。
                // 改用 SecureRandom 生成密码学安全的随机 nonce。
                nonce = ByteArray(32).also { java.security.SecureRandom().nextBytes(it) },
                timestamp = System.currentTimeMillis()
            )

            try {
                // 检查 send() 返回值 — false 表示 IPC 未连接或写入失败
                val ok = client.send(encodeScanTask(task))
                if (!ok) {
                    _uiState.value = UiState.Error("发送扫描任务失败: IPC 未连接")
                    return@launch
                }
                // 扫描超时保护 — 30s 内未收到 Report 则超时报错
                // P1-6 修复: 保存 Job 引用,在收到 Report/Alert 时取消,避免泄漏
                timeoutJob?.cancel()
                timeoutJob = launch {
                    delay(30_000L)
                    if (_uiState.value is UiState.Scanning) {
                        _uiState.value = UiState.Error("扫描超时（30s 未收到响应）")
                    }
                }
            } catch (e: Throwable) {
                _uiState.value = UiState.Error("发送扫描任务失败: ${e.message ?: e.javaClass.simpleName}")
            }
        }
    }

    private fun handleMessage(data: ByteArray) {
        val magic = data.firstOrNull() ?: return
        when (magic) {
            DetectionProtocol.MAGIC_REPORT -> parseReport(data)
            DetectionProtocol.MAGIC_ALERT -> parseAlert(data)
            DetectionProtocol.MAGIC_PROGRESS -> parseProgress(data)
        }
    }

    private fun parseReport(data: ByteArray) {
        // P1-6 修复: 收到 Report 后取消超时 Job
        timeoutJob?.cancel()
        timeoutJob = null
        val report = DetectionProtocol.decodeReport(data)
        if (report != null) {
            _uiState.value = UiState.Report(report)
        } else {
            _uiState.value = UiState.Error("Report parse failed: invalid protocol data")
        }
    }

    private fun parseAlert(data: ByteArray) {
        // P1-6 修复: 收到 Alert 后取消超时 Job (Alert 也表示扫描有响应)
        timeoutJob?.cancel()
        timeoutJob = null
        val alert = DetectionProtocol.decodeAlert(data)
        if (alert != null) {
            // 累积到历史列表（最多保留 50 条，超出丢弃最旧）
            _alerts.update { current ->
                (current + alert).takeLast(50)
            }
            // 同时设置当前活跃警报
            _uiState.value = UiState.Alert(alert)
        } else {
            _uiState.value = UiState.Error("Alert parse failed: invalid protocol data")
        }
    }

    private fun parseProgress(data: ByteArray) {
        if (data.size >= 2) {
            val pct = (data[1].toInt() and 0xFF) / 100f
            // 钳制到 [0, 1]，避免恶意/畸形数据导致进度 > 100%
            _progress.value = pct.coerceIn(0f, 1f)
        }
    }

    fun dismissReport() {
        _uiState.value = UiState.Idle
        _progress.value = 0f
    }

    /**
     * 清除当前活跃警报（UiState.Alert → Idle），但保留历史列表。
     */
    fun dismissAlert() {
        if (_uiState.value is UiState.Alert) {
            _uiState.value = UiState.Idle
        }
    }

    /**
     * 从历史列表中移除指定警报（按索引）。
     */
    fun dismissAlertFromHistory(index: Int) {
        _alerts.update { current ->
            current.toMutableList().apply {
                if (index in indices) removeAt(index)
            }.toList()
        }
    }

    /**
     * 清空所有警报历史。
     */
    fun clearAllAlerts() {
        _alerts.value = emptyList()
        if (_uiState.value is UiState.Alert) {
            _uiState.value = UiState.Idle
        }
    }

    // v1.0.2 P0-2 修复: onCleared 中用 GlobalScope launch 等待 closeAndJoin 完成
    // 旧实现 closeNow() 只 cancel 不 join,ViewModel 销毁后 readerJob 协程可能
    // 仍在运行并访问已销毁的 socket/reader → use-after-close 崩溃。
    // 新实现: 用 GlobalScope.launch(NonCancellable) 调用 closeAndJoin() 确保
    // readerJob 真正结束后再清理资源。GlobalScope 在进程级生命周期内有效,
    // 即使 ViewModel scope 已取消也能完成清理。
    override fun onCleared() {
        // 先取消超时 Job
        timeoutJob?.cancel()
        timeoutJob = null
        // 用 NonCancellable scope 确保清理协程不会被立即取消
        kotlinx.coroutines.GlobalScope.launch(kotlinx.coroutines.NonCancellable) {
            try {
                client.closeAndJoin()
            } catch (_: Throwable) {}
        }
        // 同时同步关闭作为兜底 (如果协程没来得及执行)
        try {
            client.closeNow()
        } catch (_: Throwable) {}
        super.onCleared()
    }
}

private fun encodeScanTask(task: ScanTask): ByteArray {
    val baos = ByteArrayOutputStream()
    val dos = DataOutputStream(baos)
    dos.writeByte(0x10) // task magic
    val idBytes = task.taskId.encodeToByteArray()
    dos.writeInt(minOf(idBytes.size, 64))
    dos.write(idBytes, 0, minOf(idBytes.size, 64))
    dos.writeInt(task.level.ordinal)
    dos.writeLong(task.timestamp)
    return baos.toByteArray()
}
