package com.apex.root.data.repository

import com.apex.root.data.local.SettingsDataSource
import com.apex.root.domain.model.DetectionLayerConfig
import com.apex.root.domain.model.SettingsState
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.flow.map

/**
 * 设置仓库 - 整合数据源，提供统一的设置管理接口
 */
class SettingsRepository(private val settingsDataSource: SettingsDataSource) {
    
    /**
     * 获取完整设置状态流
     */
    val settingsStateFlow: Flow<SettingsState> = combine(
        settingsDataSource.darkModeFlow,
        settingsDataSource.languageFlow,
        settingsDataSource.showNotificationsFlow,
        settingsDataSource.autoScanOnStartFlow,
        settingsDataSource.scanIntervalMinutesFlow,
        settingsDataSource.guardServiceEnabledFlow,
        settingsDataSource.selfProtectionEnabledFlow,
        settingsDataSource.loggingEnabledFlow,
        settingsDataSource.verboseLoggingFlow,
        settingsDataSource.firstLaunchFlow,
        settingsDataSource.lastScanTimestampFlow
    ) { darkMode, language, showNotifications, autoScanOnStart, scanIntervalMinutes,
        guardServiceEnabled, selfProtectionEnabled, loggingEnabled, verboseLogging,
        firstLaunch, lastScanTimestamp ->
        
        SettingsState(
            darkMode = darkMode ?: false,
            language = language ?: "zh",
            showNotifications = showNotifications ?: true,
            autoScanOnStart = autoScanOnStart ?: false,
            scanIntervalMinutes = scanIntervalMinutes ?: 30,
            guardServiceEnabled = guardServiceEnabled ?: true,
            selfProtectionEnabled = selfProtectionEnabled ?: true,
            loggingEnabled = loggingEnabled ?: true,
            verboseLogging = verboseLogging ?: false,
            firstLaunch = firstLaunch ?: true,
            lastScanTimestamp = lastScanTimestamp ?: 0L
        )
    }
    
    /**
     * 获取检测层配置流
     */
    fun getDetectionLayerConfigsFlow(): Flow<List<DetectionLayerConfig>> {
        return combine((1..12).map { layer ->
            settingsDataSource.detectionLayerEnabledFlow(layer)
                .combine(settingsDataSource.detectionLayerWeightFlow(layer)) { enabled, weight ->
                    DetectionLayerConfig(
                        layer = layer,
                        enabled = enabled ?: settingsDataSource.getDefaultDetectionLayerEnabled(layer),
                        weight = weight ?: settingsDataSource.getDefaultDetectionLayerWeight(layer)
                    )
                }
        }) { configs ->
            configs.toList()
        }
    }
    
    // 设置操作方法
    suspend fun setDarkMode(enabled: Boolean) = settingsDataSource.setDarkMode(enabled)
    
    suspend fun setLanguage(language: String) = settingsDataSource.setLanguage(language)
    
    suspend fun setShowNotifications(enabled: Boolean) = settingsDataSource.setShowNotifications(enabled)
    
    suspend fun setAutoScanOnStart(enabled: Boolean) = settingsDataSource.setAutoScanOnStart(enabled)
    
    suspend fun setScanIntervalMinutes(minutes: Int) = settingsDataSource.setScanIntervalMinutes(minutes)
    
    suspend fun setGuardServiceEnabled(enabled: Boolean) = settingsDataSource.setGuardServiceEnabled(enabled)
    
    suspend fun setSelfProtectionEnabled(enabled: Boolean) = settingsDataSource.setSelfProtectionEnabled(enabled)
    
    suspend fun setLoggingEnabled(enabled: Boolean) = settingsDataSource.setLoggingEnabled(enabled)
    
    suspend fun setVerboseLogging(enabled: Boolean) = settingsDataSource.setVerboseLogging(enabled)
    
    suspend fun setFirstLaunch(first: Boolean) = settingsDataSource.setFirstLaunch(first)
    
    suspend fun updateLastScanTimestamp() = settingsDataSource.updateLastScanTimestamp()
    
    suspend fun setDetectionLayerEnabled(layer: Int, enabled: Boolean) {
        settingsDataSource.setDetectionLayerEnabled(layer, enabled)
    }
    
    suspend fun setDetectionLayerWeight(layer: Int, weight: Int) {
        settingsDataSource.setDetectionLayerWeight(layer, weight)
    }
    
    /**
     * 获取当前设置状态（一次性）
     */
    suspend fun getCurrentSettings(): SettingsState {
        return settingsStateFlow.first()
    }
    
    /**
     * 获取当前检测层配置（一次性）
     */
    suspend fun getCurrentDetectionLayerConfigs(): List<DetectionLayerConfig> {
        return getDetectionLayerConfigsFlow().first()
    }
}
