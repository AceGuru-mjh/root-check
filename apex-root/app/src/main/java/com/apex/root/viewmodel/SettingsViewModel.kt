package com.apex.root.viewmodel

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.apex.root.data.*
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

class SettingsViewModel(application: Application) : AndroidViewModel(application) {
    private val repository = SettingsRepository(application)

    private val _settings = MutableStateFlow(repository.load())
    val settings: StateFlow<AppSettings> = _settings.asStateFlow()

    // ── Detection ──

    fun updateDetectionLevel(level: DetectionLevel) {
        _settings.value = _settings.value.copy(detectionLevel = level); persist()
    }

    fun updateAutoScan(enabled: Boolean) {
        _settings.value = _settings.value.copy(autoScanEnabled = enabled); persist()
    }

    fun updateAutoScanInterval(interval: AutoScanInterval) {
        _settings.value = _settings.value.copy(autoScanInterval = interval); persist()
    }

    fun updateRiskNotification(enabled: Boolean) {
        _settings.value = _settings.value.copy(riskNotificationEnabled = enabled); persist()
    }

    fun updateRiskThreshold(threshold: Int) {
        _settings.value = _settings.value.copy(riskThreshold = threshold.coerceIn(0, 100)); persist()
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
        _settings.value = _settings.value.copy(guardEnabled = enabled); persist()
    }

    fun updateGuardLevel(level: GuardLevel) {
        _settings.value = _settings.value.copy(guardLevel = level); persist()
    }

    fun updateGuardSelfCheckInterval(seconds: Int) {
        _settings.value = _settings.value.copy(guardSelfCheckInterval = seconds.coerceIn(1, 30)); persist()
    }

    fun updateGuardAntiDebug(enabled: Boolean) {
        _settings.value = _settings.value.copy(guardAntiDebug = enabled); persist()
    }

    fun updateGuardAutoRecovery(enabled: Boolean) {
        _settings.value = _settings.value.copy(guardAutoRecovery = enabled); persist()
    }

    // ── Hide / Spoof ──

    fun updateHideStrategy(strategy: HideStrategy) {
        _settings.value = _settings.value.copy(hideStrategy = strategy); persist()
    }

    fun updateHwidSpoof(enabled: Boolean) {
        _settings.value = _settings.value.copy(hwidSpoofEnabled = enabled); persist()
    }

    fun updateBootloaderSpoof(enabled: Boolean) {
        _settings.value = _settings.value.copy(bootloaderSpoofEnabled = enabled); persist()
    }

    // ── Game Mode ──

    fun updateGameMode(enabled: Boolean) {
        _settings.value = _settings.value.copy(gameModeEnabled = enabled); persist()
    }

    fun updateGameModeAggressive(enabled: Boolean) {
        _settings.value = _settings.value.copy(gameModeAggressive = enabled); persist()
    }

    // ── Sandbox ──

    fun updateSandboxAutoCreate(enabled: Boolean) {
        _settings.value = _settings.value.copy(sandboxAutoCreate = enabled); persist()
    }

    fun updateSandboxCleanupOnExit(enabled: Boolean) {
        _settings.value = _settings.value.copy(sandboxCleanupOnExit = enabled); persist()
    }

    // ── Cure ──

    fun updateCureDefaultLevel(level: Int) {
        _settings.value = _settings.value.copy(cureDefaultLevel = level.coerceIn(0, 3)); persist()
    }

    fun updateCureAutoFix(enabled: Boolean) {
        _settings.value = _settings.value.copy(cureAutoFix = enabled); persist()
    }

    // ── Appearance ──

    fun updateThemeMode(mode: ThemeMode) {
        _settings.value = _settings.value.copy(themeMode = mode); persist()
    }

    fun updateAccentColor(color: AccentColor) {
        _settings.value = _settings.value.copy(accentColor = color); persist()
    }

    fun updateAnimations(enabled: Boolean) {
        _settings.value = _settings.value.copy(animationsEnabled = enabled); persist()
    }

    fun updateLiquidGlassEffect(enabled: Boolean) {
        _settings.value = _settings.value.copy(liquidGlassEffect = enabled); persist()
    }

    // ── Network & Firewall ──

    fun updateEbpfFirewall(enabled: Boolean) {
        _settings.value = _settings.value.copy(ebpfFirewallEnabled = enabled); persist()
    }

    fun updateFirewallLogging(enabled: Boolean) {
        _settings.value = _settings.value.copy(firewallLogging = enabled); persist()
    }

    fun updateUpdateProxy(proxy: String) {
        _settings.value = _settings.value.copy(updateProxy = proxy); persist()
    }

    // ── Notifications ──

    fun updateNotifyScanComplete(enabled: Boolean) {
        _settings.value = _settings.value.copy(notifyScanComplete = enabled); persist()
    }

    fun updateNotifyRiskFound(enabled: Boolean) {
        _settings.value = _settings.value.copy(notifyRiskFound = enabled); persist()
    }

    fun updateNotifyGuardAlert(enabled: Boolean) {
        _settings.value = _settings.value.copy(notifyGuardAlert = enabled); persist()
    }

    fun updateNotifyCureResult(enabled: Boolean) {
        _settings.value = _settings.value.copy(notifyCureResult = enabled); persist()
    }

    fun updateNotifyUpdateAvailable(enabled: Boolean) {
        _settings.value = _settings.value.copy(notifyUpdateAvailable = enabled); persist()
    }

    // ── Privacy ──

    fun updateTelemetry(enabled: Boolean) {
        _settings.value = _settings.value.copy(telemetryEnabled = enabled); persist()
    }

    fun updateCrashReports(enabled: Boolean) {
        _settings.value = _settings.value.copy(crashReportsEnabled = enabled); persist()
    }

    // ── Updates ──

    fun updateAutoCheckUpdates(enabled: Boolean) {
        _settings.value = _settings.value.copy(autoCheckUpdates = enabled); persist()
    }

    fun updateUpdateChannel(channel: UpdateChannel) {
        _settings.value = _settings.value.copy(updateChannel = channel); persist()
    }

    fun updateWifiOnlyUpdate(enabled: Boolean) {
        _settings.value = _settings.value.copy(wifiOnlyUpdate = enabled); persist()
    }

    // ── Performance ──

    fun updateScanConcurrency(count: Int) {
        _settings.value = _settings.value.copy(scanConcurrency = count.coerceIn(1, 8)); persist()
    }

    fun updateCacheMaxAge(minutes: Int) {
        _settings.value = _settings.value.copy(cacheMaxAgeMinutes = minutes.coerceIn(1, 1440)); persist()
    }

    fun updateLowMemoryMode(enabled: Boolean) {
        _settings.value = _settings.value.copy(lowMemoryMode = enabled); persist()
    }

    // ── Logging ──

    fun updateLogLevel(level: LogLevel) {
        _settings.value = _settings.value.copy(logLevel = level); persist()
    }

    fun updateLogMaxFiles(count: Int) {
        _settings.value = _settings.value.copy(logMaxFiles = count.coerceIn(1, 50)); persist()
    }

    fun updateLogAutoCleanup(enabled: Boolean) {
        _settings.value = _settings.value.copy(logAutoCleanup = enabled); persist()
    }

    // ── Scheduling ──

    fun updateScheduleTime(time: ScanScheduleTime) {
        _settings.value = _settings.value.copy(scheduleTime = time); persist()
    }

    // ── Language ──

    fun updateLanguage(lang: Language) {
        _settings.value = _settings.value.copy(language = lang); persist()
    }

    // ── Advanced ──

    fun updateExperimentalFeatures(enabled: Boolean) {
        _settings.value = _settings.value.copy(experimentalFeatures = enabled); persist()
    }

    // ── Power ──

    fun updatePowerProfile(profile: PowerProfile) {
        _settings.value = _settings.value.copy(powerProfile = profile); persist()
    }

    fun updateBatterySaver(enabled: Boolean) {
        _settings.value = _settings.value.copy(batterySaverEnabled = enabled); persist()
    }

    // ── IPC ──

    fun updateIpcTimeout(seconds: Int) {
        _settings.value = _settings.value.copy(ipcTimeoutSeconds = seconds.coerceIn(1, 60)); persist()
    }

    fun updateIpcRetryCount(count: Int) {
        _settings.value = _settings.value.copy(ipcRetryCount = count.coerceIn(0, 20)); persist()
    }

    fun updateIpcSocketPath(path: String) {
        _settings.value = _settings.value.copy(ipcSocketPath = path); persist()
    }

    // ── Watchdog ──

    fun updateWatchdogEnabled(enabled: Boolean) {
        _settings.value = _settings.value.copy(watchdogEnabled = enabled); persist()
    }

    fun updateWatchdogInterval(seconds: Int) {
        _settings.value = _settings.value.copy(watchdogIntervalSeconds = seconds.coerceIn(5, 300)); persist()
    }

    fun updateWatchdogAutoRestart(enabled: Boolean) {
        _settings.value = _settings.value.copy(watchdogAutoRestart = enabled); persist()
    }

    // ── Anti-detection ──

    fun updateObfuscationLevel(level: ObfuscationLevel) {
        _settings.value = _settings.value.copy(obfuscationLevel = level); persist()
    }

    fun updateCpuRandomAffinity(enabled: Boolean) {
        _settings.value = _settings.value.copy(cpuRandomAffinity = enabled); persist()
    }

    // ── Whitelist & Exclusions ──

    fun updateWhitelistApps(list: String) {
        _settings.value = _settings.value.copy(whitelistApps = list); persist()
    }

    fun updateScanExclusions(list: String) {
        _settings.value = _settings.value.copy(scanExclusions = list); persist()
    }

    fun updateCustomDetectionKeywords(keywords: String) {
        _settings.value = _settings.value.copy(customDetectionKeywords = keywords); persist()
    }

    fun updateCustomHidePaths(paths: String) {
        _settings.value = _settings.value.copy(customHidePaths = paths); persist()
    }

    // ── Hardening ──

    fun updateVerifyBoot(enabled: Boolean) {
        _settings.value = _settings.value.copy(verifyBootEnabled = enabled); persist()
    }

    fun updateKeystoreCheck(enabled: Boolean) {
        _settings.value = _settings.value.copy(keystoreCheckEnabled = enabled); persist()
    }

    fun updateSelinuxEnforceCheck(enabled: Boolean) {
        _settings.value = _settings.value.copy(selinuxEnforceCheck = enabled); persist()
    }

    // ── Profile ──

    fun updateCurrentProfile(name: String) {
        _settings.value = _settings.value.copy(currentProfile = name); persist()
    }

    // ── Stealth ──

    fun updateStealthHideIcon(enabled: Boolean) {
        _settings.value = _settings.value.copy(stealthHideIcon = enabled); persist()
    }

    fun updateStealthHideRecent(enabled: Boolean) {
        _settings.value = _settings.value.copy(stealthHideRecent = enabled); persist()
    }

    fun updateStealthHideNotification(enabled: Boolean) {
        _settings.value = _settings.value.copy(stealthHideNotification = enabled); persist()
    }

    fun updateStealthFakeAppName(name: String) {
        _settings.value = _settings.value.copy(stealthFakeAppName = name); persist()
    }

    // ── Emergency ──

    fun updateEmergencyLockDevice(enabled: Boolean) {
        _settings.value = _settings.value.copy(emergencyLockDevice = enabled); persist()
    }

    fun updateEmergencyWipeData(enabled: Boolean) {
        _settings.value = _settings.value.copy(emergencyWipeData = enabled); persist()
    }

    fun updateEmergencyShutdown(enabled: Boolean) {
        _settings.value = _settings.value.copy(emergencyShutdown = enabled); persist()
    }

    fun updateEmergencyNotifyContacts(enabled: Boolean) {
        _settings.value = _settings.value.copy(emergencyNotifyContacts = enabled); persist()
    }

    // ── Side-channel Spoof ──

    fun updatePmuSpoof(enabled: Boolean) {
        _settings.value = _settings.value.copy(pmuSpoofEnabled = enabled); persist()
    }

    fun updateTimingSpoof(enabled: Boolean) {
        _settings.value = _settings.value.copy(timingSpoofEnabled = enabled); persist()
    }

    fun updateMemorySpoof(enabled: Boolean) {
        _settings.value = _settings.value.copy(memorySpoofEnabled = enabled); persist()
    }

    // ── Sound & Haptic ──

    fun updateSoundScanComplete(enabled: Boolean) {
        _settings.value = _settings.value.copy(soundScanComplete = enabled); persist()
    }

    fun updateSoundRiskAlert(enabled: Boolean) {
        _settings.value = _settings.value.copy(soundRiskAlert = enabled); persist()
    }

    fun updateSoundGuardAlert(enabled: Boolean) {
        _settings.value = _settings.value.copy(soundGuardAlert = enabled); persist()
    }

    fun updateHapticFeedback(enabled: Boolean) {
        _settings.value = _settings.value.copy(hapticFeedback = enabled); persist()
    }

    // ── App Lock ──

    fun updateAppLockEnabled(enabled: Boolean) {
        _settings.value = _settings.value.copy(appLockEnabled = enabled); persist()
    }

    fun updateAppLockPin(pin: String) {
        _settings.value = _settings.value.copy(appLockPin = pin); persist()
    }

    fun updateAppLockBiometric(enabled: Boolean) {
        _settings.value = _settings.value.copy(appLockBiometric = enabled); persist()
    }

    // ── Export ──

    fun updateAutoExport(enabled: Boolean) {
        _settings.value = _settings.value.copy(autoExportEnabled = enabled); persist()
    }

    fun updateExportFormat(format: String) {
        _settings.value = _settings.value.copy(exportFormat = format); persist()
    }

    // ── Crypto ──

    fun updateCustomEncryptionKey(key: String) {
        _settings.value = _settings.value.copy(customEncryptionKey = key); persist()
    }

    fun updateKeyRotationHours(hours: Int) {
        _settings.value = _settings.value.copy(keyRotationHours = hours.coerceIn(1, 8760)); persist()
    }

    // ── Network Monitor ──

    fun updateNetworkDnsMonitor(enabled: Boolean) {
        _settings.value = _settings.value.copy(networkDnsMonitor = enabled); persist()
    }

    fun updateNetworkTrafficMonitor(enabled: Boolean) {
        _settings.value = _settings.value.copy(networkTrafficMonitor = enabled); persist()
    }

    // ── Kernel ──

    fun updateKernelModuleCheck(enabled: Boolean) {
        _settings.value = _settings.value.copy(kernelModuleCheckEnabled = enabled); persist()
    }

    fun updateAutoSilenceOnGame(enabled: Boolean) {
        _settings.value = _settings.value.copy(autoSilenceOnGame = enabled); persist()
    }

    // ── Quick Actions ──

    fun updateQuickScanToggle(enabled: Boolean) {
        _settings.value = _settings.value.copy(quickScanToggle = enabled); persist()
    }

    fun updateFloatingWidget(enabled: Boolean) {
        _settings.value = _settings.value.copy(floatingWidget = enabled); persist()
    }

    fun updateGestureShortcut(enabled: Boolean) {
        _settings.value = _settings.value.copy(gestureShortcut = enabled); persist()
    }

    fun updateHomeScreenWidget(enabled: Boolean) {
        _settings.value = _settings.value.copy(homeScreenWidget = enabled); persist()
    }

    // ── Backup & Restore ──

    fun updateAutoBackup(enabled: Boolean) {
        _settings.value = _settings.value.copy(autoBackupEnabled = enabled); persist()
    }

    fun updateBackupIntervalDays(days: Int) {
        _settings.value = _settings.value.copy(backupIntervalDays = days); persist()
    }

    fun updateBackupPath(path: String) {
        _settings.value = _settings.value.copy(backupPath = path); persist()
    }

    fun updateCloudSync(enabled: Boolean) {
        _settings.value = _settings.value.copy(cloudSyncEnabled = enabled); persist()
    }

    // ── Connectivity ──

    fun updateUsbDetectionAlert(enabled: Boolean) {
        _settings.value = _settings.value.copy(usbDetectionAlert = enabled); persist()
    }

    fun updateWifiSecurityScan(enabled: Boolean) {
        _settings.value = _settings.value.copy(wifiSecurityScan = enabled); persist()
    }

    fun updateBleScan(enabled: Boolean) {
        _settings.value = _settings.value.copy(bleScanEnabled = enabled); persist()
    }

    fun updateUntrustedNetworkAlert(enabled: Boolean) {
        _settings.value = _settings.value.copy(untrustedNetworkAlert = enabled); persist()
    }

    // ── Automation ──

    fun updateTaskerPlugin(enabled: Boolean) {
        _settings.value = _settings.value.copy(taskerPluginEnabled = enabled); persist()
    }

    fun updateWebhookUrl(url: String) {
        _settings.value = _settings.value.copy(webhookUrl = url); persist()
    }

    fun updateIntentTrigger(enabled: Boolean) {
        _settings.value = _settings.value.copy(intentTriggerEnabled = enabled); persist()
    }

    fun updateSmsCommands(enabled: Boolean) {
        _settings.value = _settings.value.copy(smsCommandsEnabled = enabled); persist()
    }

    // ── Remote Control ──

    fun updateRemotePairing(enabled: Boolean) {
        _settings.value = _settings.value.copy(remotePairingEnabled = enabled); persist()
    }

    fun updateFcmRemoteCommands(enabled: Boolean) {
        _settings.value = _settings.value.copy(fcmRemoteCommands = enabled); persist()
    }

    fun updatePairingCode(code: String) {
        _settings.value = _settings.value.copy(pairingCode = code); persist()
    }

    fun updateAutoPairTrusted(enabled: Boolean) {
        _settings.value = _settings.value.copy(autoPairTrusted = enabled); persist()
    }

    // ── User Interface ──

    fun updateFontScale(scale: Int) {
        _settings.value = _settings.value.copy(fontScale = scale); persist()
    }

    fun updateCompactMode(enabled: Boolean) {
        _settings.value = _settings.value.copy(compactModeEnabled = enabled); persist()
    }

    fun updateStatusBarIndicator(enabled: Boolean) {
        _settings.value = _settings.value.copy(statusBarIndicator = enabled); persist()
    }

    fun updateFloatingWidgetEnabled(enabled: Boolean) {
        _settings.value = _settings.value.copy(floatingWidgetEnabled = enabled); persist()
    }

    // ── Storage ──

    fun updateReportStoragePath(path: String) {
        _settings.value = _settings.value.copy(reportStoragePath = path); persist()
    }

    fun updateCacheCleanupOnExit(enabled: Boolean) {
        _settings.value = _settings.value.copy(cacheCleanupOnExit = enabled); persist()
    }

    fun updateMaxCacheSizeMb(size: Int) {
        _settings.value = _settings.value.copy(maxCacheSizeMb = size); persist()
    }

    fun updateBackupRetentionDays(days: Int) {
        _settings.value = _settings.value.copy(backupRetentionDays = days); persist()
    }

    // ── Developer ──

    fun updateAdbMonitor(enabled: Boolean) {
        _settings.value = _settings.value.copy(adbMonitorEnabled = enabled); persist()
    }

    fun updateDevModeWarning(enabled: Boolean) {
        _settings.value = _settings.value.copy(devModeWarning = enabled); persist()
    }

    fun updateDebugLogging(enabled: Boolean) {
        _settings.value = _settings.value.copy(debugLoggingEnabled = enabled); persist()
    }

    fun updateMockLocationDetection(enabled: Boolean) {
        _settings.value = _settings.value.copy(mockLocationDetection = enabled); persist()
    }

    // ── File Monitor ──

    fun updateFileIntegrityCheck(enabled: Boolean) {
        _settings.value = _settings.value.copy(fileIntegrityCheck = enabled); persist()
    }

    fun updateInotifyWatch(enabled: Boolean) {
        _settings.value = _settings.value.copy(inotifyWatchEnabled = enabled); persist()
    }

    fun updateSystemPartitionMonitor(enabled: Boolean) {
        _settings.value = _settings.value.copy(systemPartitionMonitor = enabled); persist()
    }

    fun updateCriticalFileAlert(enabled: Boolean) {
        _settings.value = _settings.value.copy(criticalFileAlert = enabled); persist()
    }

    // ── Process ──

    fun updateSuspiciousProcessDetect(enabled: Boolean) {
        _settings.value = _settings.value.copy(suspiciousProcessDetect = enabled); persist()
    }

    fun updateProcessWhitelist(list: String) {
        _settings.value = _settings.value.copy(processWhitelist = list); persist()
    }

    fun updateBgProcessMonitor(enabled: Boolean) {
        _settings.value = _settings.value.copy(bgProcessMonitor = enabled); persist()
    }

    fun updateProcessAnomalyAlert(enabled: Boolean) {
        _settings.value = _settings.value.copy(processAnomalyAlert = enabled); persist()
    }

    // ── Battery Optimization ──

    fun updateBatteryWhitelist(enabled: Boolean) {
        _settings.value = _settings.value.copy(batteryWhitelistEnabled = enabled); persist()
    }

    fun updateDozeExempt(enabled: Boolean) {
        _settings.value = _settings.value.copy(dozeExemptEnabled = enabled); persist()
    }

    fun updateWakeLockControl(enabled: Boolean) {
        _settings.value = _settings.value.copy(wakeLockControl = enabled); persist()
    }

    fun updatePowerSaveOverride(enabled: Boolean) {
        _settings.value = _settings.value.copy(powerSaveOverride = enabled); persist()
    }

    // ── Permission ──

    fun updateAppPermissionMonitor(enabled: Boolean) {
        _settings.value = _settings.value.copy(appPermissionMonitor = enabled); persist()
    }

    fun updatePermissionChangeAlert(enabled: Boolean) {
        _settings.value = _settings.value.copy(permissionChangeAlert = enabled); persist()
    }

    fun updateSuspiciousPermissionDetect(enabled: Boolean) {
        _settings.value = _settings.value.copy(suspiciousPermissionDetect = enabled); persist()
    }

    fun updateRuntimePermissionAudit(enabled: Boolean) {
        _settings.value = _settings.value.copy(runtimePermissionAudit = enabled); persist()
    }

    // ── Sensor ──

    fun updateCameraAccessMonitor(enabled: Boolean) {
        _settings.value = _settings.value.copy(cameraAccessMonitor = enabled); persist()
    }

    fun updateMicAccessMonitor(enabled: Boolean) {
        _settings.value = _settings.value.copy(micAccessMonitor = enabled); persist()
    }

    fun updateSensorAccessLog(enabled: Boolean) {
        _settings.value = _settings.value.copy(sensorAccessLog = enabled); persist()
    }

    fun updateClipboardMonitor(enabled: Boolean) {
        _settings.value = _settings.value.copy(clipboardMonitorEnabled = enabled); persist()
    }

    // ── Audit ──

    fun updateAuditTrail(enabled: Boolean) {
        _settings.value = _settings.value.copy(auditTrailEnabled = enabled); persist()
    }

    fun updateComplianceMode(enabled: Boolean) {
        _settings.value = _settings.value.copy(complianceModeEnabled = enabled); persist()
    }

    fun updateEventRetentionDays(days: Int) {
        _settings.value = _settings.value.copy(eventRetentionDays = days); persist()
    }

    fun updateAuditExport(enabled: Boolean) {
        _settings.value = _settings.value.copy(auditExportEnabled = enabled); persist()
    }

    // ── Community ──

    fun updateCommunityForum(enabled: Boolean) {
        _settings.value = _settings.value.copy(communityForum = enabled); persist()
    }

    fun updateDiscordLink(enabled: Boolean) {
        _settings.value = _settings.value.copy(discordLink = enabled); persist()
    }

    fun updateTelegramBot(enabled: Boolean) {
        _settings.value = _settings.value.copy(telegramBotEnabled = enabled); persist()
    }

    fun updateGithubIntegration(enabled: Boolean) {
        _settings.value = _settings.value.copy(githubIntegration = enabled); persist()
    }

    // ── Experimental ──

    fun updateBetaFeatureFlags(enabled: Boolean) {
        _settings.value = _settings.value.copy(betaFeatureFlags = enabled); persist()
    }

    fun updateUnreleasedMod(enabled: Boolean) {
        _settings.value = _settings.value.copy(unreleasedModEnabled = enabled); persist()
    }

    fun updateMlDetection(enabled: Boolean) {
        _settings.value = _settings.value.copy(mlDetectionEnabled = enabled); persist()
    }

    fun updateEarlyAccessProgram(enabled: Boolean) {
        _settings.value = _settings.value.copy(earlyAccessProgram = enabled); persist()
    }

    // ── Root Management ──

    fun updateSuBinaryPath(path: String) {
        _settings.value = _settings.value.copy(suBinaryPath = path); persist()
    }

    fun updateRootAccessTimeout(seconds: Int) {
        _settings.value = _settings.value.copy(rootAccessTimeout = seconds); persist()
    }

    fun updateRootPermissionAudit(enabled: Boolean) {
        _settings.value = _settings.value.copy(rootPermissionAudit = enabled); persist()
    }

    fun updateMultiUserSupport(enabled: Boolean) {
        _settings.value = _settings.value.copy(multiUserSupport = enabled); persist()
    }

    // ── SELinux ──

    fun updateSelinuxContext(context: String) {
        _settings.value = _settings.value.copy(selinuxContext = context); persist()
    }

    fun updateSelinuxPolicyCheck(enabled: Boolean) {
        _settings.value = _settings.value.copy(selinuxPolicyCheck = enabled); persist()
    }

    fun updateSelinuxModeToggle(enabled: Boolean) {
        _settings.value = _settings.value.copy(selinuxModeToggle = enabled); persist()
    }

    fun updateContextLabelVerify(enabled: Boolean) {
        _settings.value = _settings.value.copy(contextLabelVerify = enabled); persist()
    }

    // ── Namespace ──

    fun updateMountNamespaceIsolation(enabled: Boolean) {
        _settings.value = _settings.value.copy(mountNamespaceIsolation = enabled); persist()
    }

    fun updatePidNamespace(enabled: Boolean) {
        _settings.value = _settings.value.copy(pidNamespace = enabled); persist()
    }

    fun updateNetworkNamespace(enabled: Boolean) {
        _settings.value = _settings.value.copy(networkNamespace = enabled); persist()
    }

    fun updateIpcNamespace(enabled: Boolean) {
        _settings.value = _settings.value.copy(ipcNamespace = enabled); persist()
    }

    // ── Module ──

    fun updateModuleLoadOrder(order: String) {
        _settings.value = _settings.value.copy(moduleLoadOrder = order); persist()
    }

    fun updateModuleCompatCheck(enabled: Boolean) {
        _settings.value = _settings.value.copy(moduleCompatCheck = enabled); persist()
    }

    fun updateModuleUpdatePolicy(policy: String) {
        _settings.value = _settings.value.copy(moduleUpdatePolicy = policy); persist()
    }

    fun updateModuleBlacklist(list: String) {
        _settings.value = _settings.value.copy(moduleBlacklist = list); persist()
    }

    // ── Report ──

    fun updateReportDetailLevel(level: String) {
        _settings.value = _settings.value.copy(reportDetailLevel = level); persist()
    }

    fun updateReportFormatDetailed(format: String) {
        _settings.value = _settings.value.copy(reportFormatDetailed = format); persist()
    }

    fun updateAutoReportShare(enabled: Boolean) {
        _settings.value = _settings.value.copy(autoReportShare = enabled); persist()
    }

    fun updateReportEncryption(enabled: Boolean) {
        _settings.value = _settings.value.copy(reportEncryption = enabled); persist()
    }

    // ── Secure Input ──

    fun updateSecureKeyboard(enabled: Boolean) {
        _settings.value = _settings.value.copy(secureKeyboard = enabled); persist()
    }

    fun updateInputObfuscation(enabled: Boolean) {
        _settings.value = _settings.value.copy(inputObfuscation = enabled); persist()
    }

    fun updateOverlayProtection(enabled: Boolean) {
        _settings.value = _settings.value.copy(overlayProtection = enabled); persist()
    }

    fun updateKeyloggingDetect(enabled: Boolean) {
        _settings.value = _settings.value.copy(keyloggingDetect = enabled); persist()
    }

    // ── Wireless ──

    fun updateWifiHotspotDetect(enabled: Boolean) {
        _settings.value = _settings.value.copy(wifiHotspotDetect = enabled); persist()
    }

    fun updateBluetoothTetherAlert(enabled: Boolean) {
        _settings.value = _settings.value.copy(bluetoothTetherAlert = enabled); persist()
    }

    fun updateNfcDetection(enabled: Boolean) {
        _settings.value = _settings.value.copy(nfcDetection = enabled); persist()
    }

    fun updateAirplaneModeBehavior(enabled: Boolean) {
        _settings.value = _settings.value.copy(airplaneModeBehavior = enabled); persist()
    }

    // ── Emergency Comm ──

    fun updateEmergencySmsTemplate(template: String) {
        _settings.value = _settings.value.copy(emergencySmsTemplate = template); persist()
    }

    fun updateEmergencyCall(enabled: Boolean) {
        _settings.value = _settings.value.copy(emergencyCall = enabled); persist()
    }

    fun updateLocationSharing(enabled: Boolean) {
        _settings.value = _settings.value.copy(locationSharing = enabled); persist()
    }

    fun updateHeartbeatPing(enabled: Boolean) {
        _settings.value = _settings.value.copy(heartbeatPing = enabled); persist()
    }

    // ── Play Integrity ──

    fun updatePlayIntegrityCheck(enabled: Boolean) {
        _settings.value = _settings.value.copy(playIntegrityCheck = enabled); persist()
    }

    fun updateKeyAttestation(enabled: Boolean) {
        _settings.value = _settings.value.copy(keyAttestationEnabled = enabled); persist()
    }

    fun updateCtsProfileMatch(enabled: Boolean) {
        _settings.value = _settings.value.copy(ctsProfileMatch = enabled); persist()
    }

    fun updateBasicIntegrity(enabled: Boolean) {
        _settings.value = _settings.value.copy(basicIntegrity = enabled); persist()
    }

    // ── VPN ──

    fun updateAlwaysOnVpn(enabled: Boolean) {
        _settings.value = _settings.value.copy(alwaysOnVpn = enabled); persist()
    }

    fun updateVpnKillSwitch(enabled: Boolean) {
        _settings.value = _settings.value.copy(vpnKillSwitch = enabled); persist()
    }

    fun updateDnsLeakProtection(enabled: Boolean) {
        _settings.value = _settings.value.copy(dnsLeakProtection = enabled); persist()
    }

    fun updateSplitTunneling(enabled: Boolean) {
        _settings.value = _settings.value.copy(splitTunneling = enabled); persist()
    }

    fun updateVpnProxyConfig(config: String) {
        _settings.value = _settings.value.copy(vpnProxyConfig = config); persist()
    }

    // ── Certificate ──

    fun updateCertPinning(enabled: Boolean) {
        _settings.value = _settings.value.copy(certPinningEnabled = enabled); persist()
    }

    fun updateSslVerificationLevel(level: String) {
        _settings.value = _settings.value.copy(sslVerificationLevel = level); persist()
    }

    fun updateTrustedCaList(list: String) {
        _settings.value = _settings.value.copy(trustedCaList = list); persist()
    }

    fun updateCertTransparency(enabled: Boolean) {
        _settings.value = _settings.value.copy(certTransparency = enabled); persist()
    }

    // ── Magisk ──

    fun updateMagiskVersionCheck(enabled: Boolean) {
        _settings.value = _settings.value.copy(magiskVersionCheck = enabled); persist()
    }

    fun updateMagiskModuleScan(enabled: Boolean) {
        _settings.value = _settings.value.copy(magiskModuleScan = enabled); persist()
    }

    fun updateZygiskDetect(enabled: Boolean) {
        _settings.value = _settings.value.copy(zygiskDetect = enabled); persist()
    }

    fun updateMagiskHideList(list: String) {
        _settings.value = _settings.value.copy(magiskHideList = list); persist()
    }

    // ── Xposed ──

    fun updateXposedDetect(enabled: Boolean) {
        _settings.value = _settings.value.copy(xposedDetectEnabled = enabled); persist()
    }

    fun updateXposedModuleScan(enabled: Boolean) {
        _settings.value = _settings.value.copy(xposedModuleScan = enabled); persist()
    }

    fun updateLsposedDetect(enabled: Boolean) {
        _settings.value = _settings.value.copy(lsposedDetectEnabled = enabled); persist()
    }

    fun updateRiruDetect(enabled: Boolean) {
        _settings.value = _settings.value.copy(riruDetectEnabled = enabled); persist()
    }

    // ── Script ──

    fun updateCustomInitScripts(scripts: String) {
        _settings.value = _settings.value.copy(customInitScripts = scripts); persist()
    }

    fun updatePostBootScripts(scripts: String) {
        _settings.value = _settings.value.copy(postBootScripts = scripts); persist()
    }

    fun updateScriptRunOnSchedule(enabled: Boolean) {
        _settings.value = _settings.value.copy(scriptRunOnSchedule = enabled); persist()
    }

    fun updateScriptLogOutput(enabled: Boolean) {
        _settings.value = _settings.value.copy(scriptLogOutput = enabled); persist()
    }

    // ── Hardware ──

    fun updateHardwareAttestation(enabled: Boolean) {
        _settings.value = _settings.value.copy(hardwareAttestation = enabled); persist()
    }

    fun updateTeeCheck(enabled: Boolean) {
        _settings.value = _settings.value.copy(teeCheckEnabled = enabled); persist()
    }

    fun updateSecureElementCheck(enabled: Boolean) {
        _settings.value = _settings.value.copy(secureElementCheck = enabled); persist()
    }

    fun updateFingerprintHardwareCheck(enabled: Boolean) {
        _settings.value = _settings.value.copy(fingerprintHardwareCheck = enabled); persist()
    }

    // ── System Integrity ──

    fun updateDmVerityCheck(enabled: Boolean) {
        _settings.value = _settings.value.copy(dmVerityCheck = enabled); persist()
    }

    fun updateAvbCheck(enabled: Boolean) {
        _settings.value = _settings.value.copy(avbCheck = enabled); persist()
    }

    fun updateSystemPartitionRo(enabled: Boolean) {
        _settings.value = _settings.value.copy(systemPartitionRo = enabled); persist()
    }

    fun updateVbmetaCheck(enabled: Boolean) {
        _settings.value = _settings.value.copy(vbmetaCheck = enabled); persist()
    }

    // ── Privacy Protection ──

    fun updateScreenRecordDetect(enabled: Boolean) {
        _settings.value = _settings.value.copy(screenRecordDetect = enabled); persist()
    }

    fun updateVpnDetectEnabled(enabled: Boolean) {
        _settings.value = _settings.value.copy(vpnDetectEnabled = enabled); persist()
    }

    fun updateScreenshotBlock(enabled: Boolean) {
        _settings.value = _settings.value.copy(screenshotBlock = enabled); persist()
    }

    // ── Network Security ──

    fun updateDnsOverHttps(enabled: Boolean) {
        _settings.value = _settings.value.copy(dnsOverHttps = enabled); persist()
    }

    fun updateDnsOverHttpsProvider(provider: String) {
        _settings.value = _settings.value.copy(dnsOverHttpsProvider = provider); persist()
    }

    fun updateArpSpoofDetect(enabled: Boolean) {
        _settings.value = _settings.value.copy(arpSpoofDetect = enabled); persist()
    }

    fun updateMacRandomization(enabled: Boolean) {
        _settings.value = _settings.value.copy(macRandomization = enabled); persist()
    }

    fun resetToDefaults() {
        _settings.value = AppSettings(); persist()
    }

    private fun persist() {
        viewModelScope.launch { repository.save(_settings.value) }
    }
}
