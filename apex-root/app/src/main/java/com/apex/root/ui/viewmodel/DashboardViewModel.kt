package com.apex.root.ui.viewmodel

import android.content.Context
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.apex.root.data.repository.DeviceRepository
import com.apex.root.data.repository.GuardEventRepository
import com.apex.root.data.repository.ScanRepository
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
    private val deviceRepository: DeviceRepository = DeviceRepository(),
    scanRepository: ScanRepository? = null,
    guardEventRepository: GuardEventRepository? = null
) : ViewModel() {
    
    // 如果外部未传入 Repository，则使用默认值（需要 Context）
    // 实际使用时应通过 Factory 或 Hilt 注入
    private var _scanRepository: ScanRepository? = scanRepository
    private var _guardEventRepository: GuardEventRepository? = guardEventRepository
    
    // 用于在没有 Context 时延迟初始化
    fun initializeRepositories(context: Context) {
        if (_scanRepository == null) {
            _scanRepository = ScanRepository(context)
        }
        if (_guardEventRepository == null) {
            val database = com.apex.root.data.database.ApexDatabase.getDatabase(context)
            _guardEventRepository = GuardEventRepository(database.guardEventDao())
        }
        loadRecentEvents()
    }
    
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
                _uiState.value = _uiState.value.copy(
                    scanError = "获取设备信息失败：${e.message}"
                )
            }
        }
    }
    
    private fun loadRecentEvents() {
        viewModelScope.launch {
            _guardEventRepository?.getRecentEventsFlow(20)?.collect { events ->
                _uiState.value = _uiState.value.copy(recentEvents = events)
            }
        }
    }
    
    fun onAction(action: DashboardAction) {
        when (action) {
            is DashboardAction.StartScan -> startScan()
            is DashboardAction.RefreshDevice -> loadDeviceInfo()
            is DashboardAction.DismissError -> dismissError()
        }
    }
    
    private fun startScan() {
        viewModelScope.launch {
            val scanRepo = _scanRepository ?: return@launch
            
            _uiState.value = _uiState.value.copy(
                isScanning = true,
                scanProgress = 0f
            )
            
            try {
                // 收集扫描进度
                launch {
                    scanRepo.scanProgress.collect { progress ->
                        _uiState.value = _uiState.value.copy(scanProgress = progress)
                    }
                }
                
                // 执行扫描
                val result = scanRepo.performFullScan()
                
                _uiState.value = _uiState.value.copy(
                    isScanning = false,
                    scanProgress = 1f,
                    lastScanResult = result
                )
                
                // 更新设置中的最后扫描时间
                // TODO: 通过 SettingsRepository 更新
                
            } catch (e: Exception) {
                _uiState.value = _uiState.value.copy(
                    isScanning = false,
                    scanError = "扫描失败：${e.message}"
                )
            }
        }
    }
    
    private fun dismissError() {
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
