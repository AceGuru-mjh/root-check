package com.apex.root.viewmodel

import android.app.Application
import android.util.Log
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.apex.root.data.*
import com.apex.root.data.jni.NativeBridge
import kotlinx.coroutines.CoroutineExceptionHandler
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class SettingsViewModel(application: Application) : AndroidViewModel(application) {
    private val repository = SettingsRepository(application)

    /**
     * 全局协程异常处理器 — 任何未捕获异常记录但不崩溃进程。
     * SettingsViewModel 中的 native 调用若抛出 Error (OOM 等) 不会被子调用方的 safeCall 捕获，
     * 此处兜底以防崩溃整个 App。
     */
    private val exceptionHandler = CoroutineExceptionHandler { _, e ->
        Log.e("SettingsViewModel", "Uncaught coroutine exception", e)
    }

    private val _settings = MutableStateFlow(repository.load())
    val settings: StateFlow<AppSettings> = _settings.asStateFlow()

    // ── Detection ──

    fun updateDetectionLevel(level: DetectionLevel) {
        _settings.update { it.copy(detectionLevel = level) }; persist()
    }

    fun updateAutoScan(enabled: Boolean) {
        _settings.update { it.copy(autoScanEnabled = enabled) }; persist()
    }

    fun updateAutoScanInterval(interval: AutoScanInterval) {
        _settings.update { it.copy(autoScanInterval = interval) }; persist()
    }

    fun updateRiskNotification(enabled: Boolean) {
        _settings.update { it.copy(riskNotificationEnabled = enabled) }; persist()
    }

    fun updateRiskThreshold(threshold: Int) {
        _settings.update { it.copy(riskThreshold = threshold.coerceIn(0, 100)) }; persist()
    }

    fun updatePluginEnabled(index: Int, enabled: Boolean) {
        _settings.value = when (index) {
            1 -> _settings.value.copy(pluginL1Enabled = enabled)
            2 -> _settings.value.copy(pluginL2Enabled = enabled)
            3 -> _settings.value.copy(pluginL3Enabled = enabled)
            4 -> _settings.value.copy(pluginL4Enabled = enabled)
            5 -> _settings.value.copy(pluginL5Enabled = enabled)
            6 -> _settings.value.copy(pluginL6Enabled = enabled)
            7 -> _settings.value.copy(pluginL7Enabled = enabled)
            else -> _settings.value
        }; persist()
    }

    // ── Guard ──

    fun updateGuardEnabled(enabled: Boolean) {
        _settings.update { it.copy(guardEnabled = enabled) }; persist()
    }

    fun updateGuardLevel(level: GuardLevel) {
        _settings.update { it.copy(guardLevel = level) }; persist()
    }

    fun updateGuardSelfCheckInterval(seconds: Int) {
        _settings.update { it.copy(guardSelfCheckInterval = seconds.coerceIn(1, 30)) }; persist()
    }

    fun updateGuardAntiDebug(enabled: Boolean) {
        _settings.update { it.copy(guardAntiDebug = enabled) }; persist()
    }

    fun updateGuardAutoRecovery(enabled: Boolean) {
        _settings.update { it.copy(guardAutoRecovery = enabled) }; persist()
    }

    // ── Hide / Spoof ──

    /**
     * 隐藏策略切换：UNIDIRECTIONAL / FULL / TARGETED
     * 接线：调用 NativeBridge.enableHideMode(appUid) 真正激活 native 层隐藏。
     * UNIDIRECTIONAL → 启用 Hide 模式
     * FULL → 启用 Game 模式（aggressive 隐藏）
     * TARGETED → 关闭 native 隐藏（仅依赖检测层）
     */
    fun updateHideStrategy(strategy: HideStrategy) {
        _settings.update { it.copy(hideStrategy = strategy) }; persist()
        // 接线 native 层
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            try {
                val appUid = getApplication<Application>().applicationInfo.uid
                when (strategy) {
                    HideStrategy.UNIDIRECTIONAL -> { NativeBridge.enableHideMode(appUid) }
                    HideStrategy.FULL -> { NativeBridge.enableGameMode(appUid) }
                    HideStrategy.TARGETED -> { NativeBridge.disableHideMode() }
                }
            } catch (e: Throwable) {
                Log.e("SettingsViewModel", "updateHideStrategy native call failed", e)
            }
        }
    }

    fun updateHwidSpoof(enabled: Boolean) {
        _settings.update { it.copy(hwidSpoofEnabled = enabled) }; persist()
        // 接线：HWID 伪装由 NativeHwid.spoofAll / restoreReal 控制
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            try {
                if (enabled) {
                    com.apex.root.hid.NativeHwid.spoofAll()
                } else {
                    com.apex.root.hid.NativeHwid.restoreReal()
                }
            } catch (e: Throwable) {
                Log.e("SettingsViewModel", "updateHwidSpoof native call failed", e)
            }
        }
    }

    fun updateBootloaderSpoof(enabled: Boolean) {
        _settings.update { it.copy(bootloaderSpoofEnabled = enabled) }; persist()
        // Bootloader 伪装需要 root + resetprop，由 native 层处理
        // TODO: 接入 NativeBridge.bootloaderSpoof (待实现)
    }

    // ── Game Mode ──

    /**
     * 游戏模式开关
     * 接线：调用 NativeBridge.enableGameMode / disableHideMode
     */
    fun updateGameMode(enabled: Boolean) {
        _settings.update { it.copy(gameModeEnabled = enabled) }; persist()
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            try {
                val appUid = getApplication<Application>().applicationInfo.uid
                if (enabled) {
                    NativeBridge.enableGameMode(appUid)
                } else {
                    NativeBridge.disableHideMode()
                }
            } catch (e: Throwable) {
                Log.e("SettingsViewModel", "updateGameMode native call failed", e)
            }
        }
    }

    fun updateGameModeAggressive(enabled: Boolean) {
        _settings.update { it.copy(gameModeAggressive = enabled) }; persist()
        // aggressive 模式：重新启用 game mode 以应用更激进的隐藏
        if (_settings.value.gameModeEnabled) {
            viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
                try {
                    val appUid = getApplication<Application>().applicationInfo.uid
                    NativeBridge.enableGameMode(appUid)
                } catch (e: Throwable) {
                    Log.e("SettingsViewModel", "updateGameModeAggressive native call failed", e)
                }
            }
        }
    }

    // ── Sandbox ──

    fun updateSandboxAutoCreate(enabled: Boolean) {
        _settings.update { it.copy(sandboxAutoCreate = enabled) }; persist()
    }

    fun updateSandboxCleanupOnExit(enabled: Boolean) {
        _settings.update { it.copy(sandboxCleanupOnExit = enabled) }; persist()
    }

    // ── Cure ──

    fun updateCureDefaultLevel(level: Int) {
        _settings.update { it.copy(cureDefaultLevel = level.coerceIn(0, 3)) }; persist()
    }

    fun updateCureAutoFix(enabled: Boolean) {
        _settings.update { it.copy(cureAutoFix = enabled) }; persist()
    }

    // ── Appearance ──

    fun updateThemeMode(mode: ThemeMode) {
        _settings.update { it.copy(themeMode = mode) }; persist()
    }

    fun updateAccentColor(color: AccentColor) {
        _settings.update { it.copy(accentColor = color) }; persist()
    }

    fun updateAnimations(enabled: Boolean) {
        _settings.update { it.copy(animationsEnabled = enabled) }; persist()
    }

    fun updateLiquidGlassEffect(enabled: Boolean) {
        _settings.update { it.copy(liquidGlassEffect = enabled) }; persist()
    }

    // ── Network & Firewall ──

    fun updateEbpfFirewall(enabled: Boolean) {
        _settings.update { it.copy(ebpfFirewallEnabled = enabled) }; persist()
    }

    fun updateFirewallLogging(enabled: Boolean) {
        _settings.update { it.copy(firewallLogging = enabled) }; persist()
    }

    fun updateUpdateProxy(proxy: String) {
        _settings.update { it.copy(updateProxy = proxy) }; persist()
    }

    // ── Notifications ──

    fun updateNotifyScanComplete(enabled: Boolean) {
        _settings.update { it.copy(notifyScanComplete = enabled) }; persist()
    }

    fun updateNotifyRiskFound(enabled: Boolean) {
        _settings.update { it.copy(notifyRiskFound = enabled) }; persist()
    }

    fun updateNotifyGuardAlert(enabled: Boolean) {
        _settings.update { it.copy(notifyGuardAlert = enabled) }; persist()
    }

    fun updateNotifyCureResult(enabled: Boolean) {
        _settings.update { it.copy(notifyCureResult = enabled) }; persist()
    }

    fun updateNotifyUpdateAvailable(enabled: Boolean) {
        _settings.update { it.copy(notifyUpdateAvailable = enabled) }; persist()
    }

    // ── Privacy ──

    fun updateTelemetry(enabled: Boolean) {
        _settings.update { it.copy(telemetryEnabled = enabled) }; persist()
    }

    fun updateCrashReports(enabled: Boolean) {
        _settings.update { it.copy(crashReportsEnabled = enabled) }; persist()
    }

    // ── Updates ──

    fun updateAutoCheckUpdates(enabled: Boolean) {
        _settings.update { it.copy(autoCheckUpdates = enabled) }; persist()
        // 调度或取消 WorkManager 周期性更新检查
        try {
            val app = getApplication<Application>()
            if (enabled) {
                com.apex.root.work.UpdateCheckWorker.schedule(app)
            } else {
                com.apex.root.work.UpdateCheckWorker.cancel(app)
            }
        } catch (e: Throwable) {
            android.util.Log.e("SettingsViewModel", "Failed to update WorkManager schedule", e)
        }
    }

    fun updateUpdateChannel(channel: UpdateChannel) {
        _settings.update { it.copy(updateChannel = channel) }; persist()
    }

    fun updateWifiOnlyUpdate(enabled: Boolean) {
        _settings.update { it.copy(wifiOnlyUpdate = enabled) }; persist()
    }

    // ── Performance ──

    fun updateScanConcurrency(count: Int) {
        _settings.update { it.copy(scanConcurrency = count.coerceIn(1, 8)) }; persist()
    }

    fun updateCacheMaxAge(minutes: Int) {
        _settings.update { it.copy(cacheMaxAgeMinutes = minutes.coerceIn(1, 1440)) }; persist()
    }

    fun updateLowMemoryMode(enabled: Boolean) {
        _settings.update { it.copy(lowMemoryMode = enabled) }; persist()
    }

    // ── Logging ──

    fun updateLogLevel(level: LogLevel) {
        _settings.update { it.copy(logLevel = level) }; persist()
    }

    fun updateLogMaxFiles(count: Int) {
        _settings.update { it.copy(logMaxFiles = count.coerceIn(1, 50)) }; persist()
    }

    fun updateLogAutoCleanup(enabled: Boolean) {
        _settings.update { it.copy(logAutoCleanup = enabled) }; persist()
    }

    // ── Scheduling ──

    fun updateScheduleTime(time: ScanScheduleTime) {
        _settings.update { it.copy(scheduleTime = time) }; persist()
    }

    // ── Language ──

    fun updateLanguage(lang: Language) {
        _settings.update { it.copy(language = lang) }; persist()
    }

    // ── Advanced ──

    fun updateExperimentalFeatures(enabled: Boolean) {
        _settings.update { it.copy(experimentalFeatures = enabled) }; persist()
    }

    // ── Power ──

    fun updatePowerProfile(profile: PowerProfile) {
        _settings.update { it.copy(powerProfile = profile) }; persist()
    }

    fun updateBatterySaver(enabled: Boolean) {
        _settings.update { it.copy(batterySaverEnabled = enabled) }; persist()
    }

    // ── IPC ──

    fun updateIpcTimeout(seconds: Int) {
        _settings.update { it.copy(ipcTimeoutSeconds = seconds.coerceIn(1, 60)) }; persist()
    }

    fun updateIpcRetryCount(count: Int) {
        _settings.update { it.copy(ipcRetryCount = count.coerceIn(0, 20)) }; persist()
    }

    fun updateIpcSocketPath(path: String) {
        _settings.update { it.copy(ipcSocketPath = path) }; persist()
    }

    // ── Watchdog ──

    fun updateWatchdogEnabled(enabled: Boolean) {
        _settings.update { it.copy(watchdogEnabled = enabled) }; persist()
    }

    fun updateWatchdogInterval(seconds: Int) {
        _settings.update { it.copy(watchdogIntervalSeconds = seconds.coerceIn(5, 300)) }; persist()
    }

    fun updateWatchdogAutoRestart(enabled: Boolean) {
        _settings.update { it.copy(watchdogAutoRestart = enabled) }; persist()
    }

    // ── Anti-detection ──

    fun updateObfuscationLevel(level: ObfuscationLevel) {
        _settings.update { it.copy(obfuscationLevel = level) }; persist()
    }

    fun updateCpuRandomAffinity(enabled: Boolean) {
        _settings.update { it.copy(cpuRandomAffinity = enabled) }; persist()
    }

    // ── Whitelist & Exclusions ──

    fun updateWhitelistApps(list: String) {
        _settings.update { it.copy(whitelistApps = list) }; persist()
    }

    fun updateScanExclusions(list: String) {
        _settings.update { it.copy(scanExclusions = list) }; persist()
    }

    fun updateCustomDetectionKeywords(keywords: String) {
        _settings.update { it.copy(customDetectionKeywords = keywords) }; persist()
    }

    fun updateCustomHidePaths(paths: String) {
        _settings.update { it.copy(customHidePaths = paths) }; persist()
    }

    // ── Hardening ──

    fun updateVerifyBoot(enabled: Boolean) {
        _settings.update { it.copy(verifyBootEnabled = enabled) }; persist()
    }

    fun updateKeystoreCheck(enabled: Boolean) {
        _settings.update { it.copy(keystoreCheckEnabled = enabled) }; persist()
    }

    fun updateSelinuxEnforceCheck(enabled: Boolean) {
        _settings.update { it.copy(selinuxEnforceCheck = enabled) }; persist()
    }

    // ── Profile ──

    fun updateCurrentProfile(name: String) {
        _settings.update { it.copy(currentProfile = name) }; persist()
    }

    // ── Stealth ──

    fun updateStealthHideIcon(enabled: Boolean) {
        _settings.update { it.copy(stealthHideIcon = enabled) }; persist()
    }

    fun updateStealthHideRecent(enabled: Boolean) {
        _settings.update { it.copy(stealthHideRecent = enabled) }; persist()
    }

    fun updateStealthHideNotification(enabled: Boolean) {
        _settings.update { it.copy(stealthHideNotification = enabled) }; persist()
    }

    fun updateStealthFakeAppName(name: String) {
        _settings.update { it.copy(stealthFakeAppName = name) }; persist()
    }

    // ── Emergency ──

    fun updateEmergencyLockDevice(enabled: Boolean) {
        _settings.update { it.copy(emergencyLockDevice = enabled) }; persist()
    }

    fun updateEmergencyWipeData(enabled: Boolean) {
        _settings.update { it.copy(emergencyWipeData = enabled) }; persist()
    }

    fun updateEmergencyShutdown(enabled: Boolean) {
        _settings.update { it.copy(emergencyShutdown = enabled) }; persist()
    }

    fun updateEmergencyNotifyContacts(enabled: Boolean) {
        _settings.update { it.copy(emergencyNotifyContacts = enabled) }; persist()
    }

    // ── Side-channel Spoof ──
    // 这些 spoof 由 ctrl/ 下的 C 函数实现（apex_pmu_spoof.c / apex_timing_spoof.c / apex_mem_spoof.c）
    // 当前通过 enableHideMode 间接激活；独立 JNI 接口待后续实现。
    // 启用任一 spoof 时，自动激活 Hide 模式以确保 spoof 生效。

    fun updatePmuSpoof(enabled: Boolean) {
        _settings.update { it.copy(pmuSpoofEnabled = enabled) }; persist()
        // PMU spoof 在 Hide 模式下由 apex_pmu_spoof.c::apex_pmu_spoof_read 提供
        if (enabled && !NativeBridge.isHideModeActive()) {
            viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
                try {
                    val appUid = getApplication<Application>().applicationInfo.uid
                    NativeBridge.enableHideMode(appUid)
                } catch (e: Throwable) {
                    Log.e("SettingsViewModel", "updatePmuSpoof native call failed", e)
                }
            }
        }
    }

    fun updateTimingSpoof(enabled: Boolean) {
        _settings.update { it.copy(timingSpoofEnabled = enabled) }; persist()
        // Timing spoof 由 apex_timing_spoof.c::apex_timing_adjust_clock 提供
        if (enabled && !NativeBridge.isHideModeActive()) {
            viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
                try {
                    val appUid = getApplication<Application>().applicationInfo.uid
                    NativeBridge.enableHideMode(appUid)
                } catch (e: Throwable) {
                    Log.e("SettingsViewModel", "updateTimingSpoof native call failed", e)
                }
            }
        }
    }

    fun updateMemorySpoof(enabled: Boolean) {
        _settings.update { it.copy(memorySpoofEnabled = enabled) }; persist()
        // Memory spoof 由 apex_mem_spoof.c::apex_mem_spoof_read 提供
        if (enabled && !NativeBridge.isHideModeActive()) {
            viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
                try {
                    val appUid = getApplication<Application>().applicationInfo.uid
                    NativeBridge.enableHideMode(appUid)
                } catch (e: Throwable) {
                    Log.e("SettingsViewModel", "updateMemorySpoof native call failed", e)
                }
            }
        }
    }

    // ── Sound & Haptic ──

    fun updateSoundScanComplete(enabled: Boolean) {
        _settings.update { it.copy(soundScanComplete = enabled) }; persist()
    }

    fun updateSoundRiskAlert(enabled: Boolean) {
        _settings.update { it.copy(soundRiskAlert = enabled) }; persist()
    }

    fun updateSoundGuardAlert(enabled: Boolean) {
        _settings.update { it.copy(soundGuardAlert = enabled) }; persist()
    }

    fun updateHapticFeedback(enabled: Boolean) {
        _settings.update { it.copy(hapticFeedback = enabled) }; persist()
    }

    // ── App Lock ──

    fun updateAppLockEnabled(enabled: Boolean) {
        _settings.update { it.copy(appLockEnabled = enabled) }; persist()
    }

    fun updateAppLockPin(pin: String) {
        _settings.update { it.copy(appLockPin = pin) }; persist()
    }

    fun updateAppLockBiometric(enabled: Boolean) {
        _settings.update { it.copy(appLockBiometric = enabled) }; persist()
    }

    // ── Export ──

    fun updateAutoExport(enabled: Boolean) {
        _settings.update { it.copy(autoExportEnabled = enabled) }; persist()
    }

    fun updateExportFormat(format: String) {
        _settings.update { it.copy(exportFormat = format) }; persist()
    }

    // ── Crypto ──

    fun updateCustomEncryptionKey(key: String) {
        _settings.update { it.copy(customEncryptionKey = key) }; persist()
    }

    fun updateKeyRotationHours(hours: Int) {
        _settings.update { it.copy(keyRotationHours = hours.coerceIn(1, 8760)) }; persist()
    }

    // ── Network Monitor ──

    fun updateNetworkDnsMonitor(enabled: Boolean) {
        _settings.update { it.copy(networkDnsMonitor = enabled) }; persist()
    }

    fun updateNetworkTrafficMonitor(enabled: Boolean) {
        _settings.update { it.copy(networkTrafficMonitor = enabled) }; persist()
    }

    // ── Kernel ──

    fun updateKernelModuleCheck(enabled: Boolean) {
        _settings.update { it.copy(kernelModuleCheckEnabled = enabled) }; persist()
    }

    fun updateAutoSilenceOnGame(enabled: Boolean) {
        _settings.update { it.copy(autoSilenceOnGame = enabled) }; persist()
    }

    // ── Quick Actions ──

    fun updateQuickScanToggle(enabled: Boolean) {
        _settings.update { it.copy(quickScanToggle = enabled) }; persist()
    }

    fun updateFloatingWidget(enabled: Boolean) {
        _settings.update { it.copy(floatingWidget = enabled) }; persist()
    }

    fun updateGestureShortcut(enabled: Boolean) {
        _settings.update { it.copy(gestureShortcut = enabled) }; persist()
    }

    fun updateHomeScreenWidget(enabled: Boolean) {
        _settings.update { it.copy(homeScreenWidget = enabled) }; persist()
    }

    // ── Backup & Restore ──

    fun updateAutoBackup(enabled: Boolean) {
        _settings.update { it.copy(autoBackupEnabled = enabled) }; persist()
    }

    fun updateBackupIntervalDays(days: Int) {
        _settings.update { it.copy(backupIntervalDays = days) }; persist()
    }

    fun updateBackupPath(path: String) {
        _settings.update { it.copy(backupPath = path) }; persist()
    }

    fun updateCloudSync(enabled: Boolean) {
        _settings.update { it.copy(cloudSyncEnabled = enabled) }; persist()
    }

    // ── Connectivity ──

    fun updateUsbDetectionAlert(enabled: Boolean) {
        _settings.update { it.copy(usbDetectionAlert = enabled) }; persist()
    }

    fun updateWifiSecurityScan(enabled: Boolean) {
        _settings.update { it.copy(wifiSecurityScan = enabled) }; persist()
    }

    fun updateBleScan(enabled: Boolean) {
        _settings.update { it.copy(bleScanEnabled = enabled) }; persist()
    }

    fun updateUntrustedNetworkAlert(enabled: Boolean) {
        _settings.update { it.copy(untrustedNetworkAlert = enabled) }; persist()
    }

    // ── Automation ──

    fun updateTaskerPlugin(enabled: Boolean) {
        _settings.update { it.copy(taskerPluginEnabled = enabled) }; persist()
    }

    fun updateWebhookUrl(url: String) {
        _settings.update { it.copy(webhookUrl = url) }; persist()
    }

    fun updateIntentTrigger(enabled: Boolean) {
        _settings.update { it.copy(intentTriggerEnabled = enabled) }; persist()
    }

    fun updateSmsCommands(enabled: Boolean) {
        _settings.update { it.copy(smsCommandsEnabled = enabled) }; persist()
    }

    // ── Remote Control ──

    fun updateRemotePairing(enabled: Boolean) {
        _settings.update { it.copy(remotePairingEnabled = enabled) }; persist()
    }

    fun updateFcmRemoteCommands(enabled: Boolean) {
        _settings.update { it.copy(fcmRemoteCommands = enabled) }; persist()
    }

    fun updatePairingCode(code: String) {
        _settings.update { it.copy(pairingCode = code) }; persist()
    }

    fun updateAutoPairTrusted(enabled: Boolean) {
        _settings.update { it.copy(autoPairTrusted = enabled) }; persist()
    }

    // ── User Interface ──

    fun updateFontScale(scale: Int) {
        _settings.update { it.copy(fontScale = scale) }; persist()
    }

    fun updateCompactMode(enabled: Boolean) {
        _settings.update { it.copy(compactModeEnabled = enabled) }; persist()
    }

    fun updateStatusBarIndicator(enabled: Boolean) {
        _settings.update { it.copy(statusBarIndicator = enabled) }; persist()
    }

    fun updateFloatingWidgetEnabled(enabled: Boolean) {
        _settings.update { it.copy(floatingWidgetEnabled = enabled) }; persist()
    }

    // ── Storage ──

    fun updateReportStoragePath(path: String) {
        _settings.update { it.copy(reportStoragePath = path) }; persist()
    }

    fun updateCacheCleanupOnExit(enabled: Boolean) {
        _settings.update { it.copy(cacheCleanupOnExit = enabled) }; persist()
    }

    fun updateMaxCacheSizeMb(size: Int) {
        _settings.update { it.copy(maxCacheSizeMb = size) }; persist()
    }

    fun updateBackupRetentionDays(days: Int) {
        _settings.update { it.copy(backupRetentionDays = days) }; persist()
    }

    // ── Developer ──

    fun updateAdbMonitor(enabled: Boolean) {
        _settings.update { it.copy(adbMonitorEnabled = enabled) }; persist()
    }

    fun updateDevModeWarning(enabled: Boolean) {
        _settings.update { it.copy(devModeWarning = enabled) }; persist()
    }

    fun updateDebugLogging(enabled: Boolean) {
        _settings.update { it.copy(debugLoggingEnabled = enabled) }; persist()
    }

    fun updateMockLocationDetection(enabled: Boolean) {
        _settings.update { it.copy(mockLocationDetection = enabled) }; persist()
    }

    // ── File Monitor ──

    fun updateFileIntegrityCheck(enabled: Boolean) {
        _settings.update { it.copy(fileIntegrityCheck = enabled) }; persist()
    }

    fun updateInotifyWatch(enabled: Boolean) {
        _settings.update { it.copy(inotifyWatchEnabled = enabled) }; persist()
    }

    fun updateSystemPartitionMonitor(enabled: Boolean) {
        _settings.update { it.copy(systemPartitionMonitor = enabled) }; persist()
    }

    fun updateCriticalFileAlert(enabled: Boolean) {
        _settings.update { it.copy(criticalFileAlert = enabled) }; persist()
    }

    // ── Process ──

    fun updateSuspiciousProcessDetect(enabled: Boolean) {
        _settings.update { it.copy(suspiciousProcessDetect = enabled) }; persist()
    }

    fun updateProcessWhitelist(list: String) {
        _settings.update { it.copy(processWhitelist = list) }; persist()
    }

    fun updateBgProcessMonitor(enabled: Boolean) {
        _settings.update { it.copy(bgProcessMonitor = enabled) }; persist()
    }

    fun updateProcessAnomalyAlert(enabled: Boolean) {
        _settings.update { it.copy(processAnomalyAlert = enabled) }; persist()
    }

    // ── Battery Optimization ──

    fun updateBatteryWhitelist(enabled: Boolean) {
        _settings.update { it.copy(batteryWhitelistEnabled = enabled) }; persist()
    }

    fun updateDozeExempt(enabled: Boolean) {
        _settings.update { it.copy(dozeExemptEnabled = enabled) }; persist()
    }

    fun updateWakeLockControl(enabled: Boolean) {
        _settings.update { it.copy(wakeLockControl = enabled) }; persist()
    }

    fun updatePowerSaveOverride(enabled: Boolean) {
        _settings.update { it.copy(powerSaveOverride = enabled) }; persist()
    }

    // ── Permission ──

    fun updateAppPermissionMonitor(enabled: Boolean) {
        _settings.update { it.copy(appPermissionMonitor = enabled) }; persist()
    }

    fun updatePermissionChangeAlert(enabled: Boolean) {
        _settings.update { it.copy(permissionChangeAlert = enabled) }; persist()
    }

    fun updateSuspiciousPermissionDetect(enabled: Boolean) {
        _settings.update { it.copy(suspiciousPermissionDetect = enabled) }; persist()
    }

    fun updateRuntimePermissionAudit(enabled: Boolean) {
        _settings.update { it.copy(runtimePermissionAudit = enabled) }; persist()
    }

    // ── Sensor ──

    fun updateCameraAccessMonitor(enabled: Boolean) {
        _settings.update { it.copy(cameraAccessMonitor = enabled) }; persist()
    }

    fun updateMicAccessMonitor(enabled: Boolean) {
        _settings.update { it.copy(micAccessMonitor = enabled) }; persist()
    }

    fun updateSensorAccessLog(enabled: Boolean) {
        _settings.update { it.copy(sensorAccessLog = enabled) }; persist()
    }

    fun updateClipboardMonitor(enabled: Boolean) {
        _settings.update { it.copy(clipboardMonitorEnabled = enabled) }; persist()
    }

    // ── Audit ──

    fun updateAuditTrail(enabled: Boolean) {
        _settings.update { it.copy(auditTrailEnabled = enabled) }; persist()
    }

    fun updateComplianceMode(enabled: Boolean) {
        _settings.update { it.copy(complianceModeEnabled = enabled) }; persist()
    }

    fun updateEventRetentionDays(days: Int) {
        _settings.update { it.copy(eventRetentionDays = days) }; persist()
    }

    fun updateAuditExport(enabled: Boolean) {
        _settings.update { it.copy(auditExportEnabled = enabled) }; persist()
    }

    // ── Community ──

    fun updateCommunityForum(enabled: Boolean) {
        _settings.update { it.copy(communityForum = enabled) }; persist()
    }

    fun updateDiscordLink(enabled: Boolean) {
        _settings.update { it.copy(discordLink = enabled) }; persist()
    }

    fun updateTelegramBot(enabled: Boolean) {
        _settings.update { it.copy(telegramBotEnabled = enabled) }; persist()
    }

    fun updateGithubIntegration(enabled: Boolean) {
        _settings.update { it.copy(githubIntegration = enabled) }; persist()
    }

    // ── Experimental ──

    fun updateBetaFeatureFlags(enabled: Boolean) {
        _settings.update { it.copy(betaFeatureFlags = enabled) }; persist()
    }

    fun updateUnreleasedMod(enabled: Boolean) {
        _settings.update { it.copy(unreleasedModEnabled = enabled) }; persist()
    }

    fun updateMlDetection(enabled: Boolean) {
        _settings.update { it.copy(mlDetectionEnabled = enabled) }; persist()
    }

    fun updateEarlyAccessProgram(enabled: Boolean) {
        _settings.update { it.copy(earlyAccessProgram = enabled) }; persist()
    }

    // ── Root Management ──

    fun updateSuBinaryPath(path: String) {
        _settings.update { it.copy(suBinaryPath = path) }; persist()
    }

    fun updateRootAccessTimeout(seconds: Int) {
        _settings.update { it.copy(rootAccessTimeout = seconds) }; persist()
    }

    fun updateRootPermissionAudit(enabled: Boolean) {
        _settings.update { it.copy(rootPermissionAudit = enabled) }; persist()
    }

    fun updateMultiUserSupport(enabled: Boolean) {
        _settings.update { it.copy(multiUserSupport = enabled) }; persist()
    }

    // ── SELinux ──

    fun updateSelinuxContext(context: String) {
        _settings.update { it.copy(selinuxContext = context) }; persist()
    }

    fun updateSelinuxPolicyCheck(enabled: Boolean) {
        _settings.update { it.copy(selinuxPolicyCheck = enabled) }; persist()
    }

    fun updateSelinuxModeToggle(enabled: Boolean) {
        _settings.update { it.copy(selinuxModeToggle = enabled) }; persist()
    }

    fun updateContextLabelVerify(enabled: Boolean) {
        _settings.update { it.copy(contextLabelVerify = enabled) }; persist()
    }

    // ── Namespace ──

    fun updateMountNamespaceIsolation(enabled: Boolean) {
        _settings.update { it.copy(mountNamespaceIsolation = enabled) }; persist()
    }

    fun updatePidNamespace(enabled: Boolean) {
        _settings.update { it.copy(pidNamespace = enabled) }; persist()
    }

    fun updateNetworkNamespace(enabled: Boolean) {
        _settings.update { it.copy(networkNamespace = enabled) }; persist()
    }

    fun updateIpcNamespace(enabled: Boolean) {
        _settings.update { it.copy(ipcNamespace = enabled) }; persist()
    }

    // ── Module ──

    fun updateModuleLoadOrder(order: String) {
        _settings.update { it.copy(moduleLoadOrder = order) }; persist()
    }

    fun updateModuleCompatCheck(enabled: Boolean) {
        _settings.update { it.copy(moduleCompatCheck = enabled) }; persist()
    }

    fun updateModuleUpdatePolicy(policy: String) {
        _settings.update { it.copy(moduleUpdatePolicy = policy) }; persist()
    }

    fun updateModuleBlacklist(list: String) {
        _settings.update { it.copy(moduleBlacklist = list) }; persist()
    }

    // ── Report ──

    fun updateReportDetailLevel(level: String) {
        _settings.update { it.copy(reportDetailLevel = level) }; persist()
    }

    fun updateReportFormatDetailed(format: String) {
        _settings.update { it.copy(reportFormatDetailed = format) }; persist()
    }

    fun updateAutoReportShare(enabled: Boolean) {
        _settings.update { it.copy(autoReportShare = enabled) }; persist()
    }

    fun updateReportEncryption(enabled: Boolean) {
        _settings.update { it.copy(reportEncryption = enabled) }; persist()
    }

    // ── Secure Input ──

    fun updateSecureKeyboard(enabled: Boolean) {
        _settings.update { it.copy(secureKeyboard = enabled) }; persist()
    }

    fun updateInputObfuscation(enabled: Boolean) {
        _settings.update { it.copy(inputObfuscation = enabled) }; persist()
    }

    fun updateOverlayProtection(enabled: Boolean) {
        _settings.update { it.copy(overlayProtection = enabled) }; persist()
    }

    fun updateKeyloggingDetect(enabled: Boolean) {
        _settings.update { it.copy(keyloggingDetect = enabled) }; persist()
    }

    // ── Wireless ──

    fun updateWifiHotspotDetect(enabled: Boolean) {
        _settings.update { it.copy(wifiHotspotDetect = enabled) }; persist()
    }

    fun updateBluetoothTetherAlert(enabled: Boolean) {
        _settings.update { it.copy(bluetoothTetherAlert = enabled) }; persist()
    }

    fun updateNfcDetection(enabled: Boolean) {
        _settings.update { it.copy(nfcDetection = enabled) }; persist()
    }

    fun updateAirplaneModeBehavior(enabled: Boolean) {
        _settings.update { it.copy(airplaneModeBehavior = enabled) }; persist()
    }

    // ── Emergency Comm ──

    fun updateEmergencySmsTemplate(template: String) {
        _settings.update { it.copy(emergencySmsTemplate = template) }; persist()
    }

    fun updateEmergencyCall(enabled: Boolean) {
        _settings.update { it.copy(emergencyCall = enabled) }; persist()
    }

    fun updateLocationSharing(enabled: Boolean) {
        _settings.update { it.copy(locationSharing = enabled) }; persist()
    }

    fun updateHeartbeatPing(enabled: Boolean) {
        _settings.update { it.copy(heartbeatPing = enabled) }; persist()
    }

    // ── Play Integrity ──

    fun updatePlayIntegrityCheck(enabled: Boolean) {
        _settings.update { it.copy(playIntegrityCheck = enabled) }; persist()
    }

    fun updateKeyAttestation(enabled: Boolean) {
        _settings.update { it.copy(keyAttestationEnabled = enabled) }; persist()
    }

    fun updateCtsProfileMatch(enabled: Boolean) {
        _settings.update { it.copy(ctsProfileMatch = enabled) }; persist()
    }

    fun updateBasicIntegrity(enabled: Boolean) {
        _settings.update { it.copy(basicIntegrity = enabled) }; persist()
    }

    // ── VPN ──

    fun updateAlwaysOnVpn(enabled: Boolean) {
        _settings.update { it.copy(alwaysOnVpn = enabled) }; persist()
    }

    fun updateVpnKillSwitch(enabled: Boolean) {
        _settings.update { it.copy(vpnKillSwitch = enabled) }; persist()
    }

    fun updateDnsLeakProtection(enabled: Boolean) {
        _settings.update { it.copy(dnsLeakProtection = enabled) }; persist()
    }

    fun updateSplitTunneling(enabled: Boolean) {
        _settings.update { it.copy(splitTunneling = enabled) }; persist()
    }

    fun updateVpnProxyConfig(config: String) {
        _settings.update { it.copy(vpnProxyConfig = config) }; persist()
    }

    // ── Certificate ──

    fun updateCertPinning(enabled: Boolean) {
        _settings.update { it.copy(certPinningEnabled = enabled) }; persist()
    }

    fun updateSslVerificationLevel(level: String) {
        _settings.update { it.copy(sslVerificationLevel = level) }; persist()
    }

    fun updateTrustedCaList(list: String) {
        _settings.update { it.copy(trustedCaList = list) }; persist()
    }

    fun updateCertTransparency(enabled: Boolean) {
        _settings.update { it.copy(certTransparency = enabled) }; persist()
    }

    // ── Magisk ──

    fun updateMagiskVersionCheck(enabled: Boolean) {
        _settings.update { it.copy(magiskVersionCheck = enabled) }; persist()
    }

    fun updateMagiskModuleScan(enabled: Boolean) {
        _settings.update { it.copy(magiskModuleScan = enabled) }; persist()
    }

    fun updateZygiskDetect(enabled: Boolean) {
        _settings.update { it.copy(zygiskDetect = enabled) }; persist()
    }

    fun updateMagiskHideList(list: String) {
        _settings.update { it.copy(magiskHideList = list) }; persist()
    }

    // ── Xposed ──

    fun updateXposedDetect(enabled: Boolean) {
        _settings.update { it.copy(xposedDetectEnabled = enabled) }; persist()
    }

    fun updateXposedModuleScan(enabled: Boolean) {
        _settings.update { it.copy(xposedModuleScan = enabled) }; persist()
    }

    fun updateLsposedDetect(enabled: Boolean) {
        _settings.update { it.copy(lsposedDetectEnabled = enabled) }; persist()
    }

    fun updateRiruDetect(enabled: Boolean) {
        _settings.update { it.copy(riruDetectEnabled = enabled) }; persist()
    }

    // ── Script ──

    fun updateCustomInitScripts(scripts: String) {
        _settings.update { it.copy(customInitScripts = scripts) }; persist()
    }

    fun updatePostBootScripts(scripts: String) {
        _settings.update { it.copy(postBootScripts = scripts) }; persist()
    }

    fun updateScriptRunOnSchedule(enabled: Boolean) {
        _settings.update { it.copy(scriptRunOnSchedule = enabled) }; persist()
    }

    fun updateScriptLogOutput(enabled: Boolean) {
        _settings.update { it.copy(scriptLogOutput = enabled) }; persist()
    }

    // ── Hardware ──

    fun updateHardwareAttestation(enabled: Boolean) {
        _settings.update { it.copy(hardwareAttestation = enabled) }; persist()
    }

    fun updateTeeCheck(enabled: Boolean) {
        _settings.update { it.copy(teeCheckEnabled = enabled) }; persist()
    }

    fun updateSecureElementCheck(enabled: Boolean) {
        _settings.update { it.copy(secureElementCheck = enabled) }; persist()
    }

    fun updateFingerprintHardwareCheck(enabled: Boolean) {
        _settings.update { it.copy(fingerprintHardwareCheck = enabled) }; persist()
    }

    // ── System Integrity ──

    fun updateDmVerityCheck(enabled: Boolean) {
        _settings.update { it.copy(dmVerityCheck = enabled) }; persist()
    }

    fun updateAvbCheck(enabled: Boolean) {
        _settings.update { it.copy(avbCheck = enabled) }; persist()
    }

    fun updateSystemPartitionRo(enabled: Boolean) {
        _settings.update { it.copy(systemPartitionRo = enabled) }; persist()
    }

    fun updateVbmetaCheck(enabled: Boolean) {
        _settings.update { it.copy(vbmetaCheck = enabled) }; persist()
    }

    // ── Privacy Protection ──

    fun updateScreenRecordDetect(enabled: Boolean) {
        _settings.update { it.copy(screenRecordDetect = enabled) }; persist()
    }

    fun updateVpnDetectEnabled(enabled: Boolean) {
        _settings.update { it.copy(vpnDetectEnabled = enabled) }; persist()
    }

    fun updateScreenshotBlock(enabled: Boolean) {
        _settings.update { it.copy(screenshotBlock = enabled) }; persist()
    }

    // ── Network Security ──

    fun updateDnsOverHttps(enabled: Boolean) {
        _settings.update { it.copy(dnsOverHttps = enabled) }; persist()
    }

    fun updateDnsOverHttpsProvider(provider: String) {
        _settings.update { it.copy(dnsOverHttpsProvider = provider) }; persist()
    }

    fun updateArpSpoofDetect(enabled: Boolean) {
        _settings.update { it.copy(arpSpoofDetect = enabled) }; persist()
    }

    fun updateMacRandomization(enabled: Boolean) {
        _settings.update { it.copy(macRandomization = enabled) }; persist()
    }

    fun resetToDefaults() {
        _settings.value = AppSettings(); persist()
        // 关闭所有 native 副作用，避免设置已重置但 native 状态仍激活
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            try {
                NativeBridge.disableHideMode()
                com.apex.root.hid.NativeHwid.restoreReal()
            } catch (e: Throwable) {
                Log.e("SettingsViewModel", "resetToDefaults native cleanup failed", e)
            }
        }
    }

    private fun persist() {
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            try {
                repository.save(_settings.value)
            } catch (e: Throwable) {
                Log.e("SettingsViewModel", "persist failed", e)
            }
        }
    }
}
