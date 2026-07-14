package com.apex.root.data.local

import android.content.Context
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.booleanPreferencesKey
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.intPreferencesKey
import androidx.datastore.preferences.core.longPreferencesKey
import androidx.datastore.preferences.core.stringPreferencesKey
import androidx.datastore.preferences.preferencesDataStore
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map

/**
 * DataStore 偏好设置键定义
 */
object PreferencesKeys {
    // 常规设置
    val DARK_MODE = booleanPreferencesKey("dark_mode")
    val LANGUAGE = stringPreferencesKey("language")
    val SHOW_NOTIFICATIONS = booleanPreferencesKey("show_notifications")
    
    // 检测层设置 (L1-L12)
    fun detectionLayerEnabled(layer: Int) = booleanPreferencesKey("detection_layer_$layer")
    fun detectionLayerWeight(layer: Int) = intPreferencesKey("detection_layer_weight_$layer")
    
    // 高级设置
    val AUTO_SCAN_ON_START = booleanPreferencesKey("auto_scan_on_start")
    val SCAN_INTERVAL_MINUTES = intPreferencesKey("scan_interval_minutes")
    val GUARD_SERVICE_ENABLED = booleanPreferencesKey("guard_service_enabled")
    val SELF_PROTECTION_ENABLED = booleanPreferencesKey("self_protection_enabled")
    val LOGGING_ENABLED = booleanPreferencesKey("logging_enabled")
    val VERBOSE_LOGGING = booleanPreferencesKey("verbose_logging")
    
    // 关于
    val FIRST_LAUNCH = booleanPreferencesKey("first_launch")
    val LAST_SCAN_TIMESTAMP = longPreferencesKey("last_scan_timestamp")
}

private val Context.dataStore: DataStore<Preferences> by preferencesDataStore(name = "apex_settings")

/**
 * 设置数据源 - 负责与 DataStore 交互
 */
class SettingsDataSource(private val context: Context) {
    
    val dataStore: DataStore<Preferences> = context.dataStore
    
    // 读取设置流
    val darkModeFlow: Flow<Boolean?> = dataStore.data.map { 
        it[PreferencesKeys.DARK_MODE] 
    }
    
    val languageFlow: Flow<String?> = dataStore.data.map { 
        it[PreferencesKeys.LANGUAGE] 
    }
    
    val showNotificationsFlow: Flow<Boolean?> = dataStore.data.map { 
        it[PreferencesKeys.SHOW_NOTIFICATIONS] 
    }
    
    fun detectionLayerEnabledFlow(layer: Int): Flow<Boolean?> = dataStore.data.map { 
        it[PreferencesKeys.detectionLayerEnabled(layer)] 
    }
    
    fun detectionLayerWeightFlow(layer: Int): Flow<Int?> = dataStore.data.map { 
        it[PreferencesKeys.detectionLayerWeight(layer)] 
    }
    
    val autoScanOnStartFlow: Flow<Boolean?> = dataStore.data.map { 
        it[PreferencesKeys.AUTO_SCAN_ON_START] 
    }
    
    val scanIntervalMinutesFlow: Flow<Int?> = dataStore.data.map { 
        it[PreferencesKeys.SCAN_INTERVAL_MINUTES] 
    }
    
    val guardServiceEnabledFlow: Flow<Boolean?> = dataStore.data.map { 
        it[PreferencesKeys.GUARD_SERVICE_ENABLED] 
    }
    
    val selfProtectionEnabledFlow: Flow<Boolean?> = dataStore.data.map { 
        it[PreferencesKeys.SELF_PROTECTION_ENABLED] 
    }
    
    val loggingEnabledFlow: Flow<Boolean?> = dataStore.data.map { 
        it[PreferencesKeys.LOGGING_ENABLED] 
    }
    
    val verboseLoggingFlow: Flow<Boolean?> = dataStore.data.map { 
        it[PreferencesKeys.VERBOSE_LOGGING] 
    }
    
    val firstLaunchFlow: Flow<Boolean?> = dataStore.data.map { 
        it[PreferencesKeys.FIRST_LAUNCH] 
    }
    
    val lastScanTimestampFlow: Flow<Long?> = dataStore.data.map { 
        it[PreferencesKeys.LAST_SCAN_TIMESTAMP] 
    }
    
    // 更新设置
    suspend fun setDarkMode(enabled: Boolean) {
        dataStore.edit { preferences ->
            preferences[PreferencesKeys.DARK_MODE] = enabled
        }
    }
    
    suspend fun setLanguage(language: String) {
        dataStore.edit { preferences ->
            preferences[PreferencesKeys.LANGUAGE] = language
        }
    }
    
    suspend fun setShowNotifications(enabled: Boolean) {
        dataStore.edit { preferences ->
            preferences[PreferencesKeys.SHOW_NOTIFICATIONS] = enabled
        }
    }
    
    suspend fun setDetectionLayerEnabled(layer: Int, enabled: Boolean) {
        dataStore.edit { preferences ->
            preferences[PreferencesKeys.detectionLayerEnabled(layer)] = enabled
        }
    }
    
    suspend fun setDetectionLayerWeight(layer: Int, weight: Int) {
        dataStore.edit { preferences ->
            preferences[PreferencesKeys.detectionLayerWeight(layer)] = weight
        }
    }
    
    suspend fun setAutoScanOnStart(enabled: Boolean) {
        dataStore.edit { preferences ->
            preferences[PreferencesKeys.AUTO_SCAN_ON_START] = enabled
        }
    }
    
    suspend fun setScanIntervalMinutes(minutes: Int) {
        dataStore.edit { preferences ->
            preferences[PreferencesKeys.SCAN_INTERVAL_MINUTES] = minutes
        }
    }
    
    suspend fun setGuardServiceEnabled(enabled: Boolean) {
        dataStore.edit { preferences ->
            preferences[PreferencesKeys.GUARD_SERVICE_ENABLED] = enabled
        }
    }
    
    suspend fun setSelfProtectionEnabled(enabled: Boolean) {
        dataStore.edit { preferences ->
            preferences[PreferencesKeys.SELF_PROTECTION_ENABLED] = enabled
        }
    }
    
    suspend fun setLoggingEnabled(enabled: Boolean) {
        dataStore.edit { preferences ->
            preferences[PreferencesKeys.LOGGING_ENABLED] = enabled
        }
    }
    
    suspend fun setVerboseLogging(enabled: Boolean) {
        dataStore.edit { preferences ->
            preferences[PreferencesKeys.VERBOSE_LOGGING] = enabled
        }
    }
    
    suspend fun setFirstLaunch(first: Boolean) {
        dataStore.edit { preferences ->
            preferences[PreferencesKeys.FIRST_LAUNCH] = first
        }
    }
    
    suspend fun updateLastScanTimestamp() {
        dataStore.edit { preferences ->
            preferences[PreferencesKeys.LAST_SCAN_TIMESTAMP] = System.currentTimeMillis()
        }
    }
    
    // 获取默认值
    fun getDefaultDetectionLayerEnabled(layer: Int): Boolean {
        // L1-L6 默认启用，L8-L12 默认禁用（高级检测）
        return layer <= 6
    }
    
    fun getDefaultDetectionLayerWeight(layer: Int): Int {
        // 根据层级分配权重
        return when (layer) {
            in 1..3 -> 10
            in 4..6 -> 15
            in 7..9 -> 20
            else -> 25
        }
    }
}
