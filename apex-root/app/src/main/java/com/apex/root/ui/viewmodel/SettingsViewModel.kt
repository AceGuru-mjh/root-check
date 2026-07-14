package com.apex.root.ui.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.apex.root.data.local.SettingsDataSource
import com.apex.root.data.repository.SettingsRepository
import com.apex.root.domain.model.DetectionLayerConfig
import com.apex.root.domain.model.SettingsState
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.catch
import kotlinx.coroutines.launch

/**
 * 设置页面 ViewModel
 */
class SettingsViewModel(
    private val settingsRepository: SettingsRepository
) : ViewModel() {
    
    private val _uiState = MutableStateFlow(SettingsUiState())
    val uiState: StateFlow<SettingsUiState> = _uiState.asStateFlow()
    
    init {
        loadSettings()
        loadDetectionLayerConfigs()
    }
    
    private fun loadSettings() {
        viewModelScope.launch {
            settingsRepository.settingsStateFlow
                .catch { e ->
                    _uiState.value = _uiState.value.copy(error = e.message)
                }
                .collect { settings ->
                    _uiState.value = _uiState.value.copy(
                        settings = settings,
                        isLoading = false
                    )
                }
        }
    }
    
    private fun loadDetectionLayerConfigs() {
        viewModelScope.launch {
            settingsRepository.getDetectionLayerConfigsFlow()
                .catch { e ->
                    _uiState.value = _uiState.value.copy(error = e.message)
                }
                .collect { configs ->
                    _uiState.value = _uiState.value.copy(
                        detectionLayerConfigs = configs
                    )
                }
        }
    }
    
    // 设置操作方法
    fun toggleDarkMode(enabled: Boolean) {
        viewModelScope.launch {
            settingsRepository.setDarkMode(enabled)
        }
    }
    
    fun setLanguage(language: String) {
        viewModelScope.launch {
            settingsRepository.setLanguage(language)
        }
    }
    
    fun toggleShowNotifications(enabled: Boolean) {
        viewModelScope.launch {
            settingsRepository.setShowNotifications(enabled)
        }
    }
    
    fun toggleAutoScanOnStart(enabled: Boolean) {
        viewModelScope.launch {
            settingsRepository.setAutoScanOnStart(enabled)
        }
    }
    
    fun setScanIntervalMinutes(minutes: Int) {
        viewModelScope.launch {
            settingsRepository.setScanIntervalMinutes(minutes)
        }
    }
    
    fun toggleGuardServiceEnabled(enabled: Boolean) {
        viewModelScope.launch {
            settingsRepository.setGuardServiceEnabled(enabled)
        }
    }
    
    fun toggleSelfProtectionEnabled(enabled: Boolean) {
        viewModelScope.launch {
            settingsRepository.setSelfProtectionEnabled(enabled)
        }
    }
    
    fun toggleLoggingEnabled(enabled: Boolean) {
        viewModelScope.launch {
            settingsRepository.setLoggingEnabled(enabled)
        }
    }
    
    fun toggleVerboseLogging(enabled: Boolean) {
        viewModelScope.launch {
            settingsRepository.setVerboseLogging(enabled)
        }
    }
    
    fun toggleDetectionLayer(layer: Int, enabled: Boolean) {
        viewModelScope.launch {
            settingsRepository.setDetectionLayerEnabled(layer, enabled)
        }
    }
    
    fun setDetectionLayerWeight(layer: Int, weight: Int) {
        viewModelScope.launch {
            settingsRepository.setDetectionLayerWeight(layer, weight)
        }
    }
    
    fun clearError() {
        _uiState.value = _uiState.value.copy(error = null)
    }
}

/**
 * 设置页面 UI 状态
 */
data class SettingsUiState(
    val settings: SettingsState = SettingsState(),
    val detectionLayerConfigs: List<DetectionLayerConfig> = emptyList(),
    val isLoading: Boolean = true,
    val error: String? = null
)

/**
 * ViewModel Factory
 */
class SettingsViewModelFactory(
    private val settingsRepository: SettingsRepository
) : ViewModelProvider.Factory {
    
    @Suppress("UNCHECKED_CAST")
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        if (modelClass.isAssignableFrom(SettingsViewModel::class.java)) {
            return SettingsViewModel(settingsRepository) as T
        }
        throw IllegalArgumentException("Unknown ViewModel class")
    }
}
