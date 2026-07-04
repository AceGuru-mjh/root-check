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
                nonce = ByteArray(32) { (it % 256).toByte() },
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
                // 避免用户因沙箱无响应而看到永久卡死的 Scanning 状态
                launch {
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
        val report = DetectionProtocol.decodeReport(data)
        if (report != null) {
            _uiState.value = UiState.Report(report)
        } else {
            _uiState.value = UiState.Error("Report parse failed: invalid protocol data")
        }
    }

    private fun parseAlert(data: ByteArray) {
        val alert = DetectionProtocol.decodeAlert(data)
        if (alert != null) {
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

    override fun onCleared() {
        // 修复：client.disconnect() 是 suspend fun，不能在非 suspend 的 onCleared() 直接调用。
        // 改用非挂起的 closeNow()，同步关闭 socket/reader/writer 并取消 reader job。
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
