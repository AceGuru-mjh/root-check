package com.rootdetector.config

import android.content.Context
import android.content.SharedPreferences
import com.google.gson.Gson
import com.google.gson.reflect.TypeToken

class ConfigManager private constructor(context: Context) {

    private val prefs: SharedPreferences =
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
    private val gson = Gson()

    data class StoredConfig(
        val detectionLevel: Int = 1,
        val detectionEnabledItems: Set<String> = DetectionItem.defaultsForLevel(1),
        val detectionIsCustom: Boolean = false,
        val hideLevel: Int = 1,
        val hideEnabledItems: Set<String> = HideItem.defaultsForLevel(1),
        val hideIsCustom: Boolean = false,
        val useDaemon: Boolean = true,
        val useCache: Boolean = false,
        val enableAntiHiding: Boolean = true,
        val hideAutoApply: Boolean = false,
        val hidePersistent: Boolean = false,
        val detectionTimeoutMs: Long = 30000
    )

    companion object {
        private const val PREFS_NAME = "env_detector_config"
        private const val KEY_CONFIG = "full_config"

        @Volatile
        private var instance: ConfigManager? = null

        fun getInstance(context: Context): ConfigManager {
            return instance ?: synchronized(this) {
                instance ?: ConfigManager(context.applicationContext).also { instance = it }
            }
        }
    }

    fun loadConfig(): StoredConfig {
        val json = prefs.getString(KEY_CONFIG, null) ?: return StoredConfig()
        return try {
            gson.fromJson(json, StoredConfig::class.java)
        } catch (_: Exception) {
            StoredConfig()
        }
    }

    fun saveConfig(config: StoredConfig) {
        prefs.edit().putString(KEY_CONFIG, gson.toJson(config)).apply()
    }

    fun getDetectionProfile(): DetectionProfile {
        val c = loadConfig()
        return if (c.detectionIsCustom) {
            DetectionProfile.custom(c.detectionEnabledItems)
        } else {
            DetectionProfile.preset(c.detectionLevel)
        }
    }

    fun getHideProfile(): HideProfile {
        val c = loadConfig()
        return if (c.hideIsCustom) {
            HideProfile.custom(c.hideEnabledItems)
        } else {
            HideProfile.preset(c.hideLevel)
        }
    }

    fun getDetectionConfig(): DetectionConfig {
        val c = loadConfig()
        return DetectionConfig(
            profile = if (c.detectionIsCustom) DetectionProfile.custom(c.detectionEnabledItems)
                      else DetectionProfile.preset(c.detectionLevel),
            useDaemon = c.useDaemon,
            useCache = c.useCache,
            enableAntiHiding = c.enableAntiHiding,
            timeoutMs = c.detectionTimeoutMs
        )
    }

    fun getHideConfig(): HideConfig {
        val c = loadConfig()
        return HideConfig(
            profile = if (c.hideIsCustom) HideProfile.custom(c.hideEnabledItems)
                      else HideProfile.preset(c.hideLevel),
            autoApply = c.hideAutoApply,
            persistent = c.hidePersistent
        )
    }

    fun setDetectionProfile(profile: DetectionProfile) {
        val c = loadConfig()
        saveConfig(c.copy(
            detectionLevel = profile.level,
            detectionEnabledItems = profile.enabledItems,
            detectionIsCustom = profile.isCustom
        ))
    }

    fun setHideProfile(profile: HideProfile) {
        val c = loadConfig()
        saveConfig(c.copy(
            hideLevel = profile.level,
            hideEnabledItems = profile.enabledItems,
            hideIsCustom = profile.isCustom
        ))
    }

    fun updateDetectionItems(enabledItems: Set<String>, isCustom: Boolean) {
        val c = loadConfig()
        saveConfig(c.copy(
            detectionEnabledItems = enabledItems,
            detectionIsCustom = isCustom
        ))
    }

    fun updateHideItems(enabledItems: Set<String>, isCustom: Boolean) {
        val c = loadConfig()
        saveConfig(c.copy(
            hideEnabledItems = enabledItems,
            hideIsCustom = isCustom
        ))
    }

    fun updateGlobalConfig(
        useDaemon: Boolean? = null,
        useCache: Boolean? = null,
        enableAntiHiding: Boolean? = null,
        hideAutoApply: Boolean? = null,
        hidePersistent: Boolean? = null,
        timeoutMs: Long? = null
    ) {
        val c = loadConfig()
        saveConfig(c.copy(
            useDaemon = useDaemon ?: c.useDaemon,
            useCache = useCache ?: c.useCache,
            enableAntiHiding = enableAntiHiding ?: c.enableAntiHiding,
            hideAutoApply = hideAutoApply ?: c.hideAutoApply,
            hidePersistent = hidePersistent ?: c.hidePersistent,
            detectionTimeoutMs = timeoutMs ?: c.detectionTimeoutMs
        ))
    }

    fun resetToDefaults() {
        prefs.edit().clear().apply()
    }

    fun exportConfig(): String {
        return gson.toJson(loadConfig())
    }

    fun importConfig(json: String): Boolean {
        return try {
            val config = gson.fromJson(json, StoredConfig::class.java)
            saveConfig(config)
            true
        } catch (_: Exception) {
            false
        }
    }
}
