package com.rootdetector.ui

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.rootdetector.config.*
import com.rootdetector.engine.ConfigurableDetectionEngine
import com.rootdetector.engine.ConfigurableHideEngine
import com.rootdetector.domain.model.DetectionLevel
import com.rootdetector.domain.model.ThreatLevel
import com.rootdetector.detect.DetectionEngine
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

data class ConfigUiState(
    val detectionLevel: Int = 1,
    val detectionEnabledItems: Set<String> = DetectionItem.defaultsForLevel(1),
    val detectionIsCustom: Boolean = false,
    val detectionCustomItems: Map<String, Boolean> = DetectionItem.entries.associate { it.id to it.defaultEnabled },
    val hideLevel: Int = 1,
    val hideEnabledItems: Set<String> = HideItem.defaultsForLevel(1),
    val hideIsCustom: Boolean = false,
    val hideCustomItems: Map<String, Boolean> = HideItem.entries.associate { it.id to it.defaultEnabled },
    val useDaemon: Boolean = true,
    val useCache: Boolean = false,
    val enableAntiHiding: Boolean = true,
    val hideAutoApply: Boolean = false,
    val hidePersistent: Boolean = false,
    val detectionTimeoutMs: Long = 30000,
    val isDetectionRunning: Boolean = false,
    val isHideRunning: Boolean = false,
    val detectionResult: ConfigurableDetectionEngine.DetectionResult? = null,
    val hideReport: ConfigurableHideEngine.HideReport? = null,
    val error: String? = null,
    val activeTab: Int = 0
)

class ConfigViewModel(application: Application) : AndroidViewModel(application) {

    private val configManager = ConfigManager.getInstance(application)
    private val detectionEngine = ConfigurableDetectionEngine(DetectionEngine())
    private val hideEngine = ConfigurableHideEngine()

    private val _state = MutableStateFlow(ConfigUiState())
    val state: StateFlow<ConfigUiState> = _state.asStateFlow()

    init {
        loadFromConfig()
    }

    private fun loadFromConfig() {
        val dc = configManager.getDetectionConfig()
        val hc = configManager.getHideConfig()
        val sc = configManager.loadConfig()

        _state.value = _state.value.copy(
            detectionLevel = sc.detectionLevel,
            detectionEnabledItems = sc.detectionEnabledItems,
            detectionIsCustom = sc.detectionIsCustom,
            detectionCustomItems = DetectionItem.entries.associate { item ->
                item.id to (item.id in sc.detectionEnabledItems)
            },
            hideLevel = sc.hideLevel,
            hideEnabledItems = sc.hideEnabledItems,
            hideIsCustom = sc.hideIsCustom,
            hideCustomItems = HideItem.entries.associate { item ->
                item.id to (item.id in sc.hideEnabledItems)
            },
            useDaemon = sc.useDaemon,
            useCache = sc.useCache,
            enableAntiHiding = sc.enableAntiHiding,
            hideAutoApply = sc.hideAutoApply,
            hidePersistent = sc.hidePersistent,
            detectionTimeoutMs = sc.detectionTimeoutMs
        )
    }

    fun setDetectionLevel(level: Int) {
        val profile = DetectionProfile.preset(level)
        _state.value = _state.value.copy(
            detectionLevel = level,
            detectionEnabledItems = profile.enabledItems,
            detectionIsCustom = false,
            detectionCustomItems = DetectionItem.entries.associate { item ->
                item.id to (item.id in profile.enabledItems)
            }
        )
        configManager.setDetectionProfile(profile)
    }

    fun setHideLevel(level: Int) {
        val profile = HideProfile.preset(level)
        _state.value = _state.value.copy(
            hideLevel = level,
            hideEnabledItems = profile.enabledItems,
            hideIsCustom = false,
            hideCustomItems = HideItem.entries.associate { item ->
                item.id to (item.id in profile.enabledItems)
            }
        )
        configManager.setHideProfile(profile)
    }

    fun toggleDetectionItem(itemId: String, enabled: Boolean) {
        val newItems = _state.value.detectionCustomItems.toMutableMap().apply {
            put(itemId, enabled)
        }
        val enabledSet = newItems.filter { it.value }.keys
        val matchingPreset = DetectionProfile.PRESETS.firstOrNull { it.enabledItems == enabledSet }
        if (matchingPreset != null) {
            _state.value = _state.value.copy(
                detectionLevel = matchingPreset.level,
                detectionEnabledItems = enabledSet,
                detectionIsCustom = false,
                detectionCustomItems = newItems
            )
            configManager.setDetectionProfile(matchingPreset)
        } else {
            _state.value = _state.value.copy(
                detectionEnabledItems = enabledSet,
                detectionIsCustom = true,
                detectionCustomItems = newItems
            )
            configManager.updateDetectionItems(enabledSet, true)
        }
    }

    fun toggleHideItem(itemId: String, enabled: Boolean) {
        val newItems = _state.value.hideCustomItems.toMutableMap().apply {
            put(itemId, enabled)
        }
        val enabledSet = newItems.filter { it.value }.keys
        val matchingPreset = HideProfile.PRESETS.firstOrNull { it.enabledItems == enabledSet }
        if (matchingPreset != null) {
            _state.value = _state.value.copy(
                hideLevel = matchingPreset.level,
                hideEnabledItems = enabledSet,
                hideIsCustom = false,
                hideCustomItems = newItems
            )
            configManager.setHideProfile(matchingPreset)
        } else {
            _state.value = _state.value.copy(
                hideEnabledItems = enabledSet,
                hideIsCustom = true,
                hideCustomItems = newItems
            )
            configManager.updateHideItems(enabledSet, true)
        }
    }

    fun updateGlobalSetting(
        useDaemon: Boolean? = null,
        useCache: Boolean? = null,
        enableAntiHiding: Boolean? = null,
        hideAutoApply: Boolean? = null,
        hidePersistent: Boolean? = null,
        timeoutMs: Long? = null
    ) {
        val s = _state.value
        _state.value = s.copy(
            useDaemon = useDaemon ?: s.useDaemon,
            useCache = useCache ?: s.useCache,
            enableAntiHiding = enableAntiHiding ?: s.enableAntiHiding,
            hideAutoApply = hideAutoApply ?: s.hideAutoApply,
            hidePersistent = hidePersistent ?: s.hidePersistent,
            detectionTimeoutMs = timeoutMs ?: s.detectionTimeoutMs
        )
        configManager.updateGlobalConfig(
            useDaemon = useDaemon, useCache = useCache,
            enableAntiHiding = enableAntiHiding,
            hideAutoApply = hideAutoApply, hidePersistent = hidePersistent,
            timeoutMs = timeoutMs
        )
    }

    fun runDetection() {
        if (_state.value.isDetectionRunning) return
        _state.value = _state.value.copy(isDetectionRunning = true, error = null, detectionResult = null)

        viewModelScope.launch {
            try {
                val s = _state.value
                val config = DetectionConfig(
                    profile = DetectionProfile(
                        name = if (s.detectionIsCustom) "自定义" else DetectionProfile.PRESETS.getOrNull(s.detectionLevel - 1)?.name ?: "自定义",
                        level = s.detectionLevel,
                        enabledItems = s.detectionEnabledItems,
                        isCustom = s.detectionIsCustom
                    ),
                    useDaemon = s.useDaemon,
                    useCache = s.useCache,
                    enableAntiHiding = s.enableAntiHiding,
                    timeoutMs = s.detectionTimeoutMs
                )
                val result = withContext(Dispatchers.IO) {
                    detectionEngine.runDetection(config)
                }
                _state.value = _state.value.copy(
                    isDetectionRunning = false,
                    detectionResult = result
                )
            } catch (e: Exception) {
                _state.value = _state.value.copy(
                    isDetectionRunning = false,
                    error = e.message ?: "检测失败"
                )
            }
        }
    }

    fun applyHide() {
        if (_state.value.isHideRunning) return
        _state.value = _state.value.copy(isHideRunning = true, error = null, hideReport = null)

        viewModelScope.launch {
            try {
                val s = _state.value
                val config = HideConfig(
                    profile = HideProfile(
                        name = if (s.hideIsCustom) "自定义" else HideProfile.PRESETS.getOrNull(s.hideLevel - 1)?.name ?: "自定义",
                        level = s.hideLevel,
                        enabledItems = s.hideEnabledItems,
                        isCustom = s.hideIsCustom
                    ),
                    autoApply = s.hideAutoApply,
                    persistent = s.hidePersistent
                )
                val report = withContext(Dispatchers.IO) {
                    hideEngine.applyHide(config)
                }
                _state.value = _state.value.copy(
                    isHideRunning = false,
                    hideReport = report
                )
            } catch (e: Exception) {
                _state.value = _state.value.copy(
                    isHideRunning = false,
                    error = e.message ?: "隐藏失败"
                )
            }
        }
    }

    fun revertHide() {
        if (_state.value.isHideRunning) return
        _state.value = _state.value.copy(isHideRunning = true, error = null)

        viewModelScope.launch {
            try {
                val s = _state.value
                val config = HideConfig(
                    profile = HideProfile(
                        name = "", level = s.hideLevel,
                        enabledItems = s.hideEnabledItems, isCustom = s.hideIsCustom
                    )
                )
                val report = withContext(Dispatchers.IO) {
                    hideEngine.revertHide(config)
                }
                _state.value = _state.value.copy(
                    isHideRunning = false,
                    hideReport = report
                )
            } catch (e: Exception) {
                _state.value = _state.value.copy(
                    isHideRunning = false,
                    error = e.message ?: "回滚失败"
                )
            }
        }
    }

    fun setActiveTab(tab: Int) {
        _state.value = _state.value.copy(activeTab = tab)
    }

    fun clearError() {
        _state.value = _state.value.copy(error = null)
    }

    fun clearResults() {
        _state.value = _state.value.copy(detectionResult = null, hideReport = null)
    }
}
