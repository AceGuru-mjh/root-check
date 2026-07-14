package com.apex.root.ui.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.apex.root.data.repository.DeviceRepository
import com.apex.root.domain.model.DeviceInfo
import com.apex.root.domain.model.GuardEvent
import com.apex.root.domain.model.RootDetectionResult
import com.apex.root.domain.model.ScanStatus
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

/**
 * Dashboard ViewModel
 * 负责设备信息展示、扫描控制和风险概览
 */
class DashboardViewModel(
    private val deviceRepository: DeviceRepository = DeviceRepository()
) : ViewModel() {
    
    private val _uiState = MutableStateFlow(DashboardUiState())
    val uiState: StateFlow<DashboardUiState> = _uiState.asStateFlow()
    
    init {
        loadDeviceInfo()
    }
    
    private fun loadDeviceInfo() {
        viewModelScope.launch {
            try {
                val deviceInfo = deviceRepository.getDeviceInfo()
                _uiState.value = _uiState.value.copy(deviceInfo = deviceInfo)
            } catch (e: Exception) {
                // TODO: 记录错误日志
                _uiState.value = _uiState.value.copy(
                    scanError = "获取设备信息失败：${e.message}"
                )
            }
        }
    }
    
    fun onAction(action: DashboardAction) {
        when (action) {
            is DashboardAction.StartScan -> startScan()
            is DashboardAction.RefreshDevice -> loadDeviceInfo()
            is DashboardAction.DismissError -> dismissError(action.errorId)
        }
    }
    
    private fun startScan() {
        viewModelScope.launch {
            _uiState.value = _uiState.value.copy(
                isScanning = true,
                scanProgress = 0f
            )
            
            // 模拟扫描进度
            for (i in 1..10) {
                kotlinx.coroutines.delay(100)
                _uiState.value = _uiState.value.copy(
                    scanProgress = i * 0.1f
                )
            }
            
            // 模拟扫描结果
            val mockResult = RootDetectionResult(
                isRooted = false,
                confidence = 0.95f,
                detectedMethods = emptyList(),
                riskLevel = com.apex.root.domain.model.RiskLevel.NONE
            )
            
            _uiState.value = _uiState.value.copy(
                isScanning = false,
                scanProgress = 1f,
                lastScanResult = mockResult
            )
        }
    }
    
    private fun dismissError(errorId: String) {
        _uiState.value = _uiState.value.copy(scanError = null)
    }
}

data class DashboardUiState(
    val deviceInfo: DeviceInfo? = null,
    val lastScanResult: RootDetectionResult? = null,
    val isScanning: Boolean = false,
    val scanProgress: Float = 0f,
    val scanError: String? = null,
    val recentEvents: List<GuardEvent> = emptyList()
)

sealed interface DashboardAction {
    data object StartScan : DashboardAction
    data object RefreshDevice : DashboardAction
    data class DismissError(val errorId: String) : DashboardAction
}
