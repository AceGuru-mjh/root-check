package com.apex.root.viewmodel.trusted

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.apex.root.domain.trust.model.*
import com.apex.root.ipc.DetectionProtocol
import com.apex.root.ipc.client.SecureSocketClient
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

    private val client = SecureSocketClient("apex_root_sandbox")

    init {
        viewModelScope.launch {
            client.messages.collect { data ->
                handleMessage(data)
            }
        }
        viewModelScope.launch {
            client.connectionState.collect { connected ->
                if (!connected && _uiState.value !is UiState.Idle) {
                    _uiState.value = UiState.Error("IPC connection lost")
                }
            }
        }
    }

    fun connect() {
        _uiState.value = UiState.Connecting
        viewModelScope.launch {
            val ok = client.connect()
            if (!ok) {
                _uiState.value = UiState.Error("Failed to connect to sandbox")
            } else {
                _uiState.value = UiState.Idle
            }
        }
    }

    fun startScan(level: DetectionLevel) {
        viewModelScope.launch {
            _uiState.value = UiState.Scanning
            _progress.value = 0f

            val task = ScanTask(
                taskId = UUID.randomUUID().toString(),
                level = level,
                enabledServices = listOf("*"),
                nonce = ByteArray(32) { (it % 256).toByte() },
                timestamp = System.currentTimeMillis()
            )

            client.send(encodeScanTask(task))
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
            _progress.value = pct
        }
    }

    fun dismissReport() {
        _uiState.value = UiState.Idle
        _progress.value = 0f
    }

    override fun onCleared() {
        client.disconnect()
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
