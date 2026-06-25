package com.apex.root.data

import android.content.Context
import android.content.SharedPreferences
import androidx.core.content.edit

enum class DetectionLevel(val value: Int, val label: String) {
    QUICK(0, "快速检测"),
    STANDARD(1, "标准检测"),
    DEEP(2, "深度检测"),
    FORENSIC(3, "取证检测");

    companion object {
        fun fromValue(v: Int) = entries.firstOrNull { it.value == v } ?: STANDARD
    }
}

enum class AutoScanInterval(val value: Int, val label: String) {
    NEVER(0, "从不"),
    BOOT(1, "启动时"),
    MINUTES_30(2, "每 30 分钟"),
    HOURLY(3, "每小时");

    companion object {
        fun fromValue(v: Int) = entries.firstOrNull { it.value == v } ?: BOOT
    }
}

enum class GuardLevel(val value: Int, val label: String) {
    LOW(0, "轻度防护"),
    MEDIUM(1, "标准防护"),
    HIGH(2, "严格防护");

    companion object {
        fun fromValue(v: Int) = entries.firstOrNull { it.value == v } ?: MEDIUM
    }
}

enum class HideStrategy(val value: Int, val label: String) {
    UNIDIRECTIONAL(0, "单向隐藏"),
    FULL(1, "完全隐藏"),
    TARGETED(2, "定向隐藏");

    companion object {
        fun fromValue(v: Int) = entries.firstOrNull { it.value == v } ?: UNIDIRECTIONAL
    }
}

enum class ThemeMode(val value: Int, val label: String) {
    SYSTEM(0, "跟随系统"),
    DARK(1, "深色模式"),
    LIGHT(2, "浅色模式");

    companion object {
        fun fromValue(v: Int) = entries.firstOrNull { it.value == v } ?: SYSTEM
    }
}

enum class AccentColor(val value: Int, val label: String, val hex: String) {
    PURPLE(0, "紫色", "#7C5CFC"),
    BLUE(1, "蓝色", "#5CA0FC"),
    GREEN(2, "绿色", "#4CAF50"),
    GOLD(3, "金色", "#FCD45C"),
    RED(4, "红色", "#FF5252"),
    PINK(5, "粉色", "#FF6B9D"),
    CYAN(6, "青色", "#00BCD4");

    companion object {
        fun fromValue(v: Int) = entries.firstOrNull { it.value == v } ?: PURPLE
    }
}

enum class UpdateChannel(val value: Int, val label: String) {
    STABLE(0, "稳定版"),
    BETA(1, "Beta"),
    CANARY(2, "开发版");

    companion object {
        fun fromValue(v: Int) = entries.firstOrNull { it.value == v } ?: STABLE
    }
}

enum class LogLevel(val value: Int, val label: String) {
    ERROR(0, "错误"),
    WARN(1, "警告"),
    INFO(2, "信息"),
    DEBUG(3, "调试");

    companion object {
        fun fromValue(v: Int) = entries.firstOrNull { it.value == v } ?: INFO
    }
}

enum class ScanScheduleTime(val value: Int, val label: String) {
    NEVER(0, "从不"),
    BOOT(1, "启动时"),
    DAILY_12AM(2, "每天 00:00"),
    DAILY_6AM(3, "每天 06:00"),
    DAILY_12PM(4, "每天 12:00"),
    DAILY_6PM(5, "每天 18:00");

    companion object {
        fun fromValue(v: Int) = entries.firstOrNull { it.value == v } ?: NEVER
    }
}

enum class Language(val value: Int, val label: String, val locale: String) {
    SYSTEM(0, "跟随系统", ""),
    ZH_CN(1, "简体中文", "zh"),
    EN(2, "English", "en");

    companion object {
        fun fromValue(v: Int) = entries.firstOrNull { it.value == v } ?: SYSTEM
    }
}

enum class PowerProfile(val value: Int, val label: String) {
    BALANCED(0, "均衡"),
    PERFORMANCE(1, "性能优先"),
    POWER_SAVE(2, "省电优先");

    companion object {
        fun fromValue(v: Int) = entries.firstOrNull { it.value == v } ?: BALANCED
    }
}

enum class ObfuscationLevel(val value: Int, val label: String) {
    NONE(0, "不混淆"),
    LIGHT(1, "轻度混淆"),
    STANDARD(2, "标准混淆"),
    MAXIMUM(3, "最高混淆");

    companion object {
        fun fromValue(v: Int) = entries.firstOrNull { it.value == v } ?: STANDARD
    }
}

data class AppSettings(
    val detectionLevel: DetectionLevel = DetectionLevel.STANDARD,
    val autoScanEnabled: Boolean = true,
    val autoScanInterval: AutoScanInterval = AutoScanInterval.BOOT,
    val riskNotificationEnabled: Boolean = true,
    val riskThreshold: Int = 30,
    val pluginL1Enabled: Boolean = true,
    val pluginL2Enabled: Boolean = true,
    val pluginL3Enabled: Boolean = true,
    val pluginL4Enabled: Boolean = true,
    val pluginL5Enabled: Boolean = true,
    val pluginL6Enabled: Boolean = true,
    val pluginL7Enabled: Boolean = true,
    val guardEnabled: Boolean = true,
    val guardLevel: GuardLevel = GuardLevel.MEDIUM,
    val guardSelfCheckInterval: Int = 3,
    val guardAntiDebug: Boolean = true,
    val guardAutoRecovery: Boolean = false,
    val hideStrategy: HideStrategy = HideStrategy.UNIDIRECTIONAL,
    val hwidSpoofEnabled: Boolean = false,
    val bootloaderSpoofEnabled: Boolean = false,
    val gameModeEnabled: Boolean = false,
    val gameModeAggressive: Boolean = false,
    val sandboxAutoCreate: Boolean = false,
    val sandboxCleanupOnExit: Boolean = true,
    val cureDefaultLevel: Int = 0,
    val cureAutoFix: Boolean = false,
    val themeMode: ThemeMode = ThemeMode.SYSTEM,
    val accentColor: AccentColor = AccentColor.PURPLE,
    val animationsEnabled: Boolean = true,
    val liquidGlassEffect: Boolean = true,
    val ebpfFirewallEnabled: Boolean = true,
    val firewallLogging: Boolean = false,
    val updateProxy: String = "",
    val notifyScanComplete: Boolean = true,
    val notifyRiskFound: Boolean = true,
    val notifyGuardAlert: Boolean = true,
    val notifyCureResult: Boolean = true,
    val notifyUpdateAvailable: Boolean = true,
    val telemetryEnabled: Boolean = false,
    val crashReportsEnabled: Boolean = true,
    val autoCheckUpdates: Boolean = true,
    val updateChannel: UpdateChannel = UpdateChannel.STABLE,
    val wifiOnlyUpdate: Boolean = true,
    val scanConcurrency: Int = 2,
    val cacheMaxAgeMinutes: Int = 30,
    val lowMemoryMode: Boolean = false,
    val logLevel: LogLevel = LogLevel.INFO,
    val logMaxFiles: Int = 5,
    val logAutoCleanup: Boolean = true,
    val scheduleTime: ScanScheduleTime = ScanScheduleTime.NEVER,
    val language: Language = Language.SYSTEM,
    val experimentalFeatures: Boolean = false,
    val powerProfile: PowerProfile = PowerProfile.BALANCED,
    val batterySaverEnabled: Boolean = false,
    val ipcTimeoutSeconds: Int = 5,
    val ipcRetryCount: Int = 3,
    val ipcSocketPath: String = "",
    val watchdogEnabled: Boolean = true,
    val watchdogIntervalSeconds: Int = 30,
    val watchdogAutoRestart: Boolean = false,
    val obfuscationLevel: ObfuscationLevel = ObfuscationLevel.STANDARD,
    val cpuRandomAffinity: Boolean = true,
    val whitelistApps: String = "",
    val scanExclusions: String = "",
    val customDetectionKeywords: String = "",
    val customHidePaths: String = "",
    val verifyBootEnabled: Boolean = true,
    val keystoreCheckEnabled: Boolean = true,
    val selinuxEnforceCheck: Boolean = true,
    val currentProfile: String = "default",
    val stealthHideIcon: Boolean = false,
    val stealthHideRecent: Boolean = false,
    val stealthHideNotification: Boolean = false,
    val stealthFakeAppName: String = "",
    val emergencyLockDevice: Boolean = false,
    val emergencyWipeData: Boolean = false,
    val emergencyShutdown: Boolean = false,
    val emergencyNotifyContacts: Boolean = false,
    val pmuSpoofEnabled: Boolean = false,
    val timingSpoofEnabled: Boolean = false,
    val memorySpoofEnabled: Boolean = false,
    val soundScanComplete: Boolean = true,
    val soundRiskAlert: Boolean = true,
    val soundGuardAlert: Boolean = true,
    val hapticFeedback: Boolean = true,
    val appLockEnabled: Boolean = false,
    val appLockPin: String = "",
    val appLockBiometric: Boolean = false,
    val autoExportEnabled: Boolean = false,
    val exportFormat: String = "json",
    val customEncryptionKey: String = "",
    val keyRotationHours: Int = 168,
    val networkDnsMonitor: Boolean = false,
    val networkTrafficMonitor: Boolean = false,
    val kernelModuleCheckEnabled: Boolean = true,
    val autoSilenceOnGame: Boolean = true,
    val quickScanToggle: Boolean = true,
    val floatingWidget: Boolean = false,
    val gestureShortcut: Boolean = false,
    val homeScreenWidget: Boolean = false,
    val autoBackupEnabled: Boolean = false,
    val backupIntervalDays: Int = 7,
    val backupPath: String = "",
    val cloudSyncEnabled: Boolean = false,
    val usbDetectionAlert: Boolean = true,
    val wifiSecurityScan: Boolean = true,
    val bleScanEnabled: Boolean = false,
    val untrustedNetworkAlert: Boolean = true,
    val taskerPluginEnabled: Boolean = false,
    val webhookUrl: String = "",
    val intentTriggerEnabled: Boolean = false,
    val smsCommandsEnabled: Boolean = false,
    val remotePairingEnabled: Boolean = false,
    val fcmRemoteCommands: Boolean = false,
    val pairingCode: String = "",
    val autoPairTrusted: Boolean = true,
    val fontScale: Int = 100,
    val compactModeEnabled: Boolean = false,
    val statusBarIndicator: Boolean = false,
    val floatingWidgetEnabled: Boolean = false,
    val reportStoragePath: String = "",
    val cacheCleanupOnExit: Boolean = true,
    val maxCacheSizeMb: Int = 50,
    val backupRetentionDays: Int = 30,
    val adbMonitorEnabled: Boolean = true,
    val devModeWarning: Boolean = true,
    val debugLoggingEnabled: Boolean = false,
    val mockLocationDetection: Boolean = true,
    val fileIntegrityCheck: Boolean = true,
    val inotifyWatchEnabled: Boolean = false,
    val systemPartitionMonitor: Boolean = true,
    val criticalFileAlert: Boolean = true,
    val suspiciousProcessDetect: Boolean = true,
    val processWhitelist: String = "",
    val bgProcessMonitor: Boolean = false,
    val processAnomalyAlert: Boolean = true,
    val batteryWhitelistEnabled: Boolean = false,
    val dozeExemptEnabled: Boolean = false,
    val wakeLockControl: Boolean = false,
    val powerSaveOverride: Boolean = false,
    val appPermissionMonitor: Boolean = true,
    val permissionChangeAlert: Boolean = true,
    val suspiciousPermissionDetect: Boolean = true,
    val runtimePermissionAudit: Boolean = false,
    val cameraAccessMonitor: Boolean = true,
    val micAccessMonitor: Boolean = true,
    val sensorAccessLog: Boolean = false,
    val clipboardMonitorEnabled: Boolean = false,
    val auditTrailEnabled: Boolean = true,
    val complianceModeEnabled: Boolean = false,
    val eventRetentionDays: Int = 30,
    val auditExportEnabled: Boolean = false,
    val communityForum: Boolean = true,
    val discordLink: Boolean = true,
    val telegramBotEnabled: Boolean = false,
    val githubIntegration: Boolean = false,
    val betaFeatureFlags: Boolean = false,
    val unreleasedModEnabled: Boolean = false,
    val mlDetectionEnabled: Boolean = false,
    val earlyAccessProgram: Boolean = false,
    val suBinaryPath: String = "",
    val rootAccessTimeout: Int = 30,
    val rootPermissionAudit: Boolean = false,
    val multiUserSupport: Boolean = true,
    val selinuxContext: String = "",
    val selinuxPolicyCheck: Boolean = true,
    val selinuxModeToggle: Boolean = false,
    val contextLabelVerify: Boolean = true,
    val mountNamespaceIsolation: Boolean = false,
    val pidNamespace: Boolean = false,
    val networkNamespace: Boolean = false,
    val ipcNamespace: Boolean = false,
    val moduleLoadOrder: String = "",
    val moduleCompatCheck: Boolean = true,
    val moduleUpdatePolicy: String = "stable",
    val moduleBlacklist: String = "",
    val reportDetailLevel: String = "standard",
    val reportFormatDetailed: String = "pdf",
    val autoReportShare: Boolean = false,
    val reportEncryption: Boolean = true,
    val secureKeyboard: Boolean = false,
    val inputObfuscation: Boolean = false,
    val overlayProtection: Boolean = true,
    val keyloggingDetect: Boolean = true,
    val wifiHotspotDetect: Boolean = true,
    val bluetoothTetherAlert: Boolean = true,
    val nfcDetection: Boolean = false,
    val airplaneModeBehavior: Boolean = false,
    val emergencySmsTemplate: String = "",
    val emergencyCall: Boolean = false,
    val locationSharing: Boolean = false,
    val heartbeatPing: Boolean = false,
    val playIntegrityCheck: Boolean = true,
    val keyAttestationEnabled: Boolean = true,
    val ctsProfileMatch: Boolean = true,
    val basicIntegrity: Boolean = true,
    val alwaysOnVpn: Boolean = false,
    val vpnKillSwitch: Boolean = false,
    val dnsLeakProtection: Boolean = true,
    val splitTunneling: Boolean = false,
    val vpnProxyConfig: String = "",
    val certPinningEnabled: Boolean = true,
    val sslVerificationLevel: String = "strict",
    val trustedCaList: String = "",
    val certTransparency: Boolean = false,
    val magiskVersionCheck: Boolean = true,
    val magiskModuleScan: Boolean = true,
    val zygiskDetect: Boolean = true,
    val magiskHideList: String = "",
    val xposedDetectEnabled: Boolean = true,
    val xposedModuleScan: Boolean = true,
    val lsposedDetectEnabled: Boolean = true,
    val riruDetectEnabled: Boolean = true,
    val customInitScripts: String = "",
    val postBootScripts: String = "",
    val scriptRunOnSchedule: Boolean = false,
    val scriptLogOutput: Boolean = false,
    val hardwareAttestation: Boolean = true,
    val teeCheckEnabled: Boolean = true,
    val secureElementCheck: Boolean = true,
    val fingerprintHardwareCheck: Boolean = true,
    val dmVerityCheck: Boolean = true,
    val avbCheck: Boolean = true,
    val systemPartitionRo: Boolean = true,
    val vbmetaCheck: Boolean = true,
    // Privacy Protection
    val screenRecordDetect: Boolean = true,
    val vpnDetectEnabled: Boolean = true,
    val screenshotBlock: Boolean = false,
    // Network Security
    val dnsOverHttps: Boolean = false,
    val dnsOverHttpsProvider: String = "cloudflare",
    val arpSpoofDetect: Boolean = true,
    val macRandomization: Boolean = false
)

class SettingsRepository(context: Context) {
    private val prefs: SharedPreferences =
        context.getSharedPreferences("apex_root_settings", Context.MODE_PRIVATE)

    fun load(): AppSettings = AppSettings(
        detectionLevel = DetectionLevel.fromValue(prefs.getInt(KEY_DETECTION_LEVEL, DetectionLevel.STANDARD.value)),
        autoScanEnabled = prefs.getBoolean(KEY_AUTO_SCAN, true),
        autoScanInterval = AutoScanInterval.fromValue(prefs.getInt(KEY_AUTO_SCAN_INTERVAL, AutoScanInterval.BOOT.value)),
        riskNotificationEnabled = prefs.getBoolean(KEY_RISK_NOTIFICATION, true),
        riskThreshold = prefs.getInt(KEY_RISK_THRESHOLD, 30),
        pluginL1Enabled = prefs.getBoolean(KEY_PLUGIN_L1, true),
        pluginL2Enabled = prefs.getBoolean(KEY_PLUGIN_L2, true),
        pluginL3Enabled = prefs.getBoolean(KEY_PLUGIN_L3, true),
        pluginL4Enabled = prefs.getBoolean(KEY_PLUGIN_L4, true),
        pluginL5Enabled = prefs.getBoolean(KEY_PLUGIN_L5, true),
        pluginL6Enabled = prefs.getBoolean(KEY_PLUGIN_L6, true),
        pluginL7Enabled = prefs.getBoolean(KEY_PLUGIN_L7, true),
        guardEnabled = prefs.getBoolean(KEY_GUARD_ENABLED, true),
        guardLevel = GuardLevel.fromValue(prefs.getInt(KEY_GUARD_LEVEL, GuardLevel.MEDIUM.value)),
        guardSelfCheckInterval = prefs.getInt(KEY_GUARD_SELF_CHECK_INTERVAL, 3),
        guardAntiDebug = prefs.getBoolean(KEY_GUARD_ANTI_DEBUG, true),
        guardAutoRecovery = prefs.getBoolean(KEY_GUARD_AUTO_RECOVERY, false),
        hideStrategy = HideStrategy.fromValue(prefs.getInt(KEY_HIDE_STRATEGY, HideStrategy.UNIDIRECTIONAL.value)),
        hwidSpoofEnabled = prefs.getBoolean(KEY_HWID_SPOOF, false),
        bootloaderSpoofEnabled = prefs.getBoolean(KEY_BOOTLOADER_SPOOF, false),
        gameModeEnabled = prefs.getBoolean(KEY_GAME_MODE, false),
        gameModeAggressive = prefs.getBoolean(KEY_GAME_MODE_AGGRESSIVE, false),
        sandboxAutoCreate = prefs.getBoolean(KEY_SANDBOX_AUTO_CREATE, false),
        sandboxCleanupOnExit = prefs.getBoolean(KEY_SANDBOX_CLEANUP, true),
        cureDefaultLevel = prefs.getInt(KEY_CURE_DEFAULT_LEVEL, 0),
        cureAutoFix = prefs.getBoolean(KEY_CURE_AUTO_FIX, false),
        themeMode = ThemeMode.fromValue(prefs.getInt(KEY_THEME_MODE, ThemeMode.SYSTEM.value)),
        accentColor = AccentColor.fromValue(prefs.getInt(KEY_ACCENT_COLOR, AccentColor.PURPLE.value)),
        animationsEnabled = prefs.getBoolean(KEY_ANIMATIONS, true),
        liquidGlassEffect = prefs.getBoolean(KEY_LIQUID_GLASS, true),
        ebpfFirewallEnabled = prefs.getBoolean(KEY_EBPF_FIREWALL, true),
        firewallLogging = prefs.getBoolean(KEY_FIREWALL_LOGGING, false),
        updateProxy = prefs.getString(KEY_UPDATE_PROXY, "") ?: "",
        notifyScanComplete = prefs.getBoolean(KEY_NOTIFY_SCAN_COMPLETE, true),
        notifyRiskFound = prefs.getBoolean(KEY_NOTIFY_RISK_FOUND, true),
        notifyGuardAlert = prefs.getBoolean(KEY_NOTIFY_GUARD_ALERT, true),
        notifyCureResult = prefs.getBoolean(KEY_NOTIFY_CURE_RESULT, true),
        notifyUpdateAvailable = prefs.getBoolean(KEY_NOTIFY_UPDATE_AVAILABLE, true),
        telemetryEnabled = prefs.getBoolean(KEY_TELEMETRY, false),
        crashReportsEnabled = prefs.getBoolean(KEY_CRASH_REPORTS, true),
        autoCheckUpdates = prefs.getBoolean(KEY_AUTO_CHECK_UPDATES, true),
        updateChannel = UpdateChannel.fromValue(prefs.getInt(KEY_UPDATE_CHANNEL, UpdateChannel.STABLE.value)),
        wifiOnlyUpdate = prefs.getBoolean(KEY_WIFI_ONLY_UPDATE, true),
        scanConcurrency = prefs.getInt(KEY_SCAN_CONCURRENCY, 2),
        cacheMaxAgeMinutes = prefs.getInt(KEY_CACHE_MAX_AGE, 30),
        lowMemoryMode = prefs.getBoolean(KEY_LOW_MEMORY_MODE, false),
        logLevel = LogLevel.fromValue(prefs.getInt(KEY_LOG_LEVEL, LogLevel.INFO.value)),
        logMaxFiles = prefs.getInt(KEY_LOG_MAX_FILES, 5),
        logAutoCleanup = prefs.getBoolean(KEY_LOG_AUTO_CLEANUP, true),
        scheduleTime = ScanScheduleTime.fromValue(prefs.getInt(KEY_SCHEDULE_TIME, ScanScheduleTime.NEVER.value)),
        language = Language.fromValue(prefs.getInt(KEY_LANGUAGE, Language.SYSTEM.value)),
        experimentalFeatures = prefs.getBoolean(KEY_EXPERIMENTAL, false),
        powerProfile = PowerProfile.fromValue(prefs.getInt(KEY_POWER_PROFILE, PowerProfile.BALANCED.value)),
        batterySaverEnabled = prefs.getBoolean(KEY_BATTERY_SAVER, false),
        ipcTimeoutSeconds = prefs.getInt(KEY_IPC_TIMEOUT, 5),
        ipcRetryCount = prefs.getInt(KEY_IPC_RETRY, 3),
        ipcSocketPath = prefs.getString(KEY_IPC_SOCKET_PATH, "") ?: "",
        watchdogEnabled = prefs.getBoolean(KEY_WATCHDOG_ENABLED, true),
        watchdogIntervalSeconds = prefs.getInt(KEY_WATCHDOG_INTERVAL, 30),
        watchdogAutoRestart = prefs.getBoolean(KEY_WATCHDOG_AUTO_RESTART, false),
        obfuscationLevel = ObfuscationLevel.fromValue(prefs.getInt(KEY_OBFUSCATION_LEVEL, ObfuscationLevel.STANDARD.value)),
        cpuRandomAffinity = prefs.getBoolean(KEY_CPU_RANDOM_AFFINITY, true),
        whitelistApps = prefs.getString(KEY_WHITELIST_APPS, "") ?: "",
        scanExclusions = prefs.getString(KEY_SCAN_EXCLUSIONS, "") ?: "",
        customDetectionKeywords = prefs.getString(KEY_CUSTOM_KEYWORDS, "") ?: "",
        customHidePaths = prefs.getString(KEY_CUSTOM_HIDE_PATHS, "") ?: "",
        verifyBootEnabled = prefs.getBoolean(KEY_VERIFY_BOOT, true),
        keystoreCheckEnabled = prefs.getBoolean(KEY_KEYSTORE_CHECK, true),
        selinuxEnforceCheck = prefs.getBoolean(KEY_SELINUX_ENFORCE, true),
        currentProfile = prefs.getString(KEY_CURRENT_PROFILE, "default") ?: "default",
        stealthHideIcon = prefs.getBoolean(KEY_STEALTH_HIDE_ICON, false),
        stealthHideRecent = prefs.getBoolean(KEY_STEALTH_HIDE_RECENT, false),
        stealthHideNotification = prefs.getBoolean(KEY_STEALTH_HIDE_NOTIFICATION, false),
        stealthFakeAppName = prefs.getString(KEY_STEALTH_FAKE_APP_NAME, "") ?: "",
        emergencyLockDevice = prefs.getBoolean(KEY_EMERGENCY_LOCK, false),
        emergencyWipeData = prefs.getBoolean(KEY_EMERGENCY_WIPE, false),
        emergencyShutdown = prefs.getBoolean(KEY_EMERGENCY_SHUTDOWN, false),
        emergencyNotifyContacts = prefs.getBoolean(KEY_EMERGENCY_NOTIFY, false),
        pmuSpoofEnabled = prefs.getBoolean(KEY_PMU_SPOOF, false),
        timingSpoofEnabled = prefs.getBoolean(KEY_TIMING_SPOOF, false),
        memorySpoofEnabled = prefs.getBoolean(KEY_MEMORY_SPOOF, false),
        soundScanComplete = prefs.getBoolean(KEY_SOUND_SCAN_COMPLETE, true),
        soundRiskAlert = prefs.getBoolean(KEY_SOUND_RISK_ALERT, true),
        soundGuardAlert = prefs.getBoolean(KEY_SOUND_GUARD_ALERT, true),
        hapticFeedback = prefs.getBoolean(KEY_HAPTIC_FEEDBACK, true),
        appLockEnabled = prefs.getBoolean(KEY_APP_LOCK_ENABLED, false),
        appLockPin = prefs.getString(KEY_APP_LOCK_PIN, "") ?: "",
        appLockBiometric = prefs.getBoolean(KEY_APP_LOCK_BIOMETRIC, false),
        autoExportEnabled = prefs.getBoolean(KEY_AUTO_EXPORT, false),
        exportFormat = prefs.getString(KEY_EXPORT_FORMAT, "json") ?: "json",
        customEncryptionKey = prefs.getString(KEY_CUSTOM_ENCRYPTION_KEY, "") ?: "",
        keyRotationHours = prefs.getInt(KEY_KEY_ROTATION_HOURS, 168),
        networkDnsMonitor = prefs.getBoolean(KEY_NETWORK_DNS_MONITOR, false),
        networkTrafficMonitor = prefs.getBoolean(KEY_NETWORK_TRAFFIC_MONITOR, false),
        kernelModuleCheckEnabled = prefs.getBoolean(KEY_KERNEL_MODULE_CHECK, true),
        autoSilenceOnGame = prefs.getBoolean(KEY_AUTO_SILENCE_ON_GAME, true),
        quickScanToggle = prefs.getBoolean(KEY_QUICK_SCAN_TOGGLE, true),
        floatingWidget = prefs.getBoolean(KEY_FLOATING_WIDGET, false),
        gestureShortcut = prefs.getBoolean(KEY_GESTURE_SHORTCUT, false),
        homeScreenWidget = prefs.getBoolean(KEY_HOME_SCREEN_WIDGET, false),
        autoBackupEnabled = prefs.getBoolean(KEY_AUTO_BACKUP, false),
        backupIntervalDays = prefs.getInt(KEY_BACKUP_INTERVAL_DAYS, 7),
        backupPath = prefs.getString(KEY_BACKUP_PATH, "") ?: "",
        cloudSyncEnabled = prefs.getBoolean(KEY_CLOUD_SYNC, false),
        usbDetectionAlert = prefs.getBoolean(KEY_USB_DETECTION_ALERT, true),
        wifiSecurityScan = prefs.getBoolean(KEY_WIFI_SECURITY_SCAN, true),
        bleScanEnabled = prefs.getBoolean(KEY_BLE_SCAN, false),
        untrustedNetworkAlert = prefs.getBoolean(KEY_UNTRUSTED_NETWORK_ALERT, true),
        taskerPluginEnabled = prefs.getBoolean(KEY_TASKER_PLUGIN, false),
        webhookUrl = prefs.getString(KEY_WEBHOOK_URL, "") ?: "",
        intentTriggerEnabled = prefs.getBoolean(KEY_INTENT_TRIGGER, false),
        smsCommandsEnabled = prefs.getBoolean(KEY_SMS_COMMANDS, false),
        remotePairingEnabled = prefs.getBoolean(KEY_REMOTE_PAIRING, false),
        fcmRemoteCommands = prefs.getBoolean(KEY_FCM_REMOTE_COMMANDS, false),
        pairingCode = prefs.getString(KEY_PAIRING_CODE, "") ?: "",
        autoPairTrusted = prefs.getBoolean(KEY_AUTO_PAIR_TRUSTED, true),
        fontScale = prefs.getInt(KEY_FONT_SCALE, 100),
        compactModeEnabled = prefs.getBoolean(KEY_COMPACT_MODE, false),
        statusBarIndicator = prefs.getBoolean(KEY_STATUS_BAR_INDICATOR, false),
        floatingWidgetEnabled = prefs.getBoolean(KEY_FLOATING_WIDGET_ENABLED, false),
        reportStoragePath = prefs.getString(KEY_REPORT_STORAGE_PATH, "") ?: "",
        cacheCleanupOnExit = prefs.getBoolean(KEY_CACHE_CLEANUP_ON_EXIT, true),
        maxCacheSizeMb = prefs.getInt(KEY_MAX_CACHE_SIZE_MB, 50),
        backupRetentionDays = prefs.getInt(KEY_BACKUP_RETENTION_DAYS, 30),
        adbMonitorEnabled = prefs.getBoolean(KEY_ADB_MONITOR, true),
        devModeWarning = prefs.getBoolean(KEY_DEV_MODE_WARNING, true),
        debugLoggingEnabled = prefs.getBoolean(KEY_DEBUG_LOGGING, false),
        mockLocationDetection = prefs.getBoolean(KEY_MOCK_LOCATION_DETECTION, true),
        fileIntegrityCheck = prefs.getBoolean(KEY_FILE_INTEGRITY_CHECK, true),
        inotifyWatchEnabled = prefs.getBoolean(KEY_INOTIFY_WATCH, false),
        systemPartitionMonitor = prefs.getBoolean(KEY_SYSTEM_PARTITION_MONITOR, true),
        criticalFileAlert = prefs.getBoolean(KEY_CRITICAL_FILE_ALERT, true),
        suspiciousProcessDetect = prefs.getBoolean(KEY_SUSPICIOUS_PROCESS_DETECT, true),
        processWhitelist = prefs.getString(KEY_PROCESS_WHITELIST, "") ?: "",
        bgProcessMonitor = prefs.getBoolean(KEY_BG_PROCESS_MONITOR, false),
        processAnomalyAlert = prefs.getBoolean(KEY_PROCESS_ANOMALY_ALERT, true),
        batteryWhitelistEnabled = prefs.getBoolean(KEY_BATTERY_WHITELIST, false),
        dozeExemptEnabled = prefs.getBoolean(KEY_DOZE_EXEMPT, false),
        wakeLockControl = prefs.getBoolean(KEY_WAKE_LOCK_CONTROL, false),
        powerSaveOverride = prefs.getBoolean(KEY_POWER_SAVE_OVERRIDE, false),
        appPermissionMonitor = prefs.getBoolean(KEY_APP_PERMISSION_MONITOR, true),
        permissionChangeAlert = prefs.getBoolean(KEY_PERMISSION_CHANGE_ALERT, true),
        suspiciousPermissionDetect = prefs.getBoolean(KEY_SUSPICIOUS_PERMISSION_DETECT, true),
        runtimePermissionAudit = prefs.getBoolean(KEY_RUNTIME_PERMISSION_AUDIT, false),
        cameraAccessMonitor = prefs.getBoolean(KEY_CAMERA_ACCESS_MONITOR, true),
        micAccessMonitor = prefs.getBoolean(KEY_MIC_ACCESS_MONITOR, true),
        sensorAccessLog = prefs.getBoolean(KEY_SENSOR_ACCESS_LOG, false),
        clipboardMonitorEnabled = prefs.getBoolean(KEY_CLIPBOARD_MONITOR, false),
        auditTrailEnabled = prefs.getBoolean(KEY_AUDIT_TRAIL, true),
        complianceModeEnabled = prefs.getBoolean(KEY_COMPLIANCE_MODE, false),
        eventRetentionDays = prefs.getInt(KEY_EVENT_RETENTION_DAYS, 30),
        auditExportEnabled = prefs.getBoolean(KEY_AUDIT_EXPORT, false),
        communityForum = prefs.getBoolean(KEY_COMMUNITY_FORUM, true),
        discordLink = prefs.getBoolean(KEY_DISCORD_LINK, true),
        telegramBotEnabled = prefs.getBoolean(KEY_TELEGRAM_BOT, false),
        githubIntegration = prefs.getBoolean(KEY_GITHUB_INTEGRATION, false),
        betaFeatureFlags = prefs.getBoolean(KEY_BETA_FEATURE_FLAGS, false),
        unreleasedModEnabled = prefs.getBoolean(KEY_UNRELEASED_MOD, false),
        mlDetectionEnabled = prefs.getBoolean(KEY_ML_DETECTION, false),
        earlyAccessProgram = prefs.getBoolean(KEY_EARLY_ACCESS, false),
        suBinaryPath = prefs.getString(KEY_SU_BINARY_PATH, "") ?: "",
        rootAccessTimeout = prefs.getInt(KEY_ROOT_ACCESS_TIMEOUT, 30),
        rootPermissionAudit = prefs.getBoolean(KEY_ROOT_PERMISSION_AUDIT, false),
        multiUserSupport = prefs.getBoolean(KEY_MULTI_USER_SUPPORT, true),
        selinuxContext = prefs.getString(KEY_SELINUX_CONTEXT, "") ?: "",
        selinuxPolicyCheck = prefs.getBoolean(KEY_SELINUX_POLICY_CHECK, true),
        selinuxModeToggle = prefs.getBoolean(KEY_SELINUX_MODE_TOGGLE, false),
        contextLabelVerify = prefs.getBoolean(KEY_CONTEXT_LABEL_VERIFY, true),
        mountNamespaceIsolation = prefs.getBoolean(KEY_MOUNT_NS_ISOLATION, false),
        pidNamespace = prefs.getBoolean(KEY_PID_NAMESPACE, false),
        networkNamespace = prefs.getBoolean(KEY_NETWORK_NAMESPACE, false),
        ipcNamespace = prefs.getBoolean(KEY_IPC_NAMESPACE, false),
        moduleLoadOrder = prefs.getString(KEY_MODULE_LOAD_ORDER, "") ?: "",
        moduleCompatCheck = prefs.getBoolean(KEY_MODULE_COMPAT_CHECK, true),
        moduleUpdatePolicy = prefs.getString(KEY_MODULE_UPDATE_POLICY, "stable") ?: "stable",
        moduleBlacklist = prefs.getString(KEY_MODULE_BLACKLIST, "") ?: "",
        reportDetailLevel = prefs.getString(KEY_REPORT_DETAIL_LEVEL, "standard") ?: "standard",
        reportFormatDetailed = prefs.getString(KEY_REPORT_FORMAT_DETAILED, "pdf") ?: "pdf",
        autoReportShare = prefs.getBoolean(KEY_AUTO_REPORT_SHARE, false),
        reportEncryption = prefs.getBoolean(KEY_REPORT_ENCRYPTION, true),
        secureKeyboard = prefs.getBoolean(KEY_SECURE_KEYBOARD, false),
        inputObfuscation = prefs.getBoolean(KEY_INPUT_OBFUSCATION, false),
        overlayProtection = prefs.getBoolean(KEY_OVERLAY_PROTECTION, true),
        keyloggingDetect = prefs.getBoolean(KEY_KEYLOGGING_DETECT, true),
        wifiHotspotDetect = prefs.getBoolean(KEY_WIFI_HOTSPOT_DETECT, true),
        bluetoothTetherAlert = prefs.getBoolean(KEY_BLUETOOTH_TETHER_ALERT, true),
        nfcDetection = prefs.getBoolean(KEY_NFC_DETECTION, false),
        airplaneModeBehavior = prefs.getBoolean(KEY_AIRPLANE_MODE_BEHAVIOR, false),
        emergencySmsTemplate = prefs.getString(KEY_EMERGENCY_SMS_TEMPLATE, "") ?: "",
        emergencyCall = prefs.getBoolean(KEY_EMERGENCY_CALL, false),
        locationSharing = prefs.getBoolean(KEY_LOCATION_SHARING, false),
        heartbeatPing = prefs.getBoolean(KEY_HEARTBEAT_PING, false),
        playIntegrityCheck = prefs.getBoolean(KEY_PLAY_INTEGRITY, true),
        keyAttestationEnabled = prefs.getBoolean(KEY_KEY_ATTESTATION, true),
        ctsProfileMatch = prefs.getBoolean(KEY_CTS_PROFILE, true),
        basicIntegrity = prefs.getBoolean(KEY_BASIC_INTEGRITY, true),
        alwaysOnVpn = prefs.getBoolean(KEY_ALWAYS_ON_VPN, false),
        vpnKillSwitch = prefs.getBoolean(KEY_VPN_KILL_SWITCH, false),
        dnsLeakProtection = prefs.getBoolean(KEY_DNS_LEAK_PROTECTION, true),
        splitTunneling = prefs.getBoolean(KEY_SPLIT_TUNNELING, false),
        vpnProxyConfig = prefs.getString(KEY_VPN_PROXY_CONFIG, "") ?: "",
        certPinningEnabled = prefs.getBoolean(KEY_CERT_PINNING, true),
        sslVerificationLevel = prefs.getString(KEY_SSL_VERIFICATION_LEVEL, "strict") ?: "strict",
        trustedCaList = prefs.getString(KEY_TRUSTED_CA_LIST, "") ?: "",
        certTransparency = prefs.getBoolean(KEY_CERT_TRANSPARENCY, false),
        magiskVersionCheck = prefs.getBoolean(KEY_MAGISK_VERSION_CHECK, true),
        magiskModuleScan = prefs.getBoolean(KEY_MAGISK_MODULE_SCAN, true),
        zygiskDetect = prefs.getBoolean(KEY_ZYGISK_DETECT, true),
        magiskHideList = prefs.getString(KEY_MAGISK_HIDE_LIST, "") ?: "",
        xposedDetectEnabled = prefs.getBoolean(KEY_XPOSED_DETECT, true),
        xposedModuleScan = prefs.getBoolean(KEY_XPOSED_MODULE_SCAN, true),
        lsposedDetectEnabled = prefs.getBoolean(KEY_LSPOSED_DETECT, true),
        riruDetectEnabled = prefs.getBoolean(KEY_RIRU_DETECT, true),
        customInitScripts = prefs.getString(KEY_CUSTOM_INIT_SCRIPTS, "") ?: "",
        postBootScripts = prefs.getString(KEY_POST_BOOT_SCRIPTS, "") ?: "",
        scriptRunOnSchedule = prefs.getBoolean(KEY_SCRIPT_SCHEDULE, false),
        scriptLogOutput = prefs.getBoolean(KEY_SCRIPT_LOG_OUTPUT, false),
        hardwareAttestation = prefs.getBoolean(KEY_HARDWARE_ATTESTATION, true),
        teeCheckEnabled = prefs.getBoolean(KEY_TEE_CHECK, true),
        secureElementCheck = prefs.getBoolean(KEY_SECURE_ELEMENT, true),
        fingerprintHardwareCheck = prefs.getBoolean(KEY_FINGERPRINT_HARDWARE, true),
        dmVerityCheck = prefs.getBoolean(KEY_DM_VERITY, true),
        avbCheck = prefs.getBoolean(KEY_AVB, true),
        systemPartitionRo = prefs.getBoolean(KEY_SYSTEM_PARTITION_RO, true),
        vbmetaCheck = prefs.getBoolean(KEY_VBMETA, true),
        screenRecordDetect = prefs.getBoolean(KEY_SCREEN_RECORD_DETECT, true),
        vpnDetectEnabled = prefs.getBoolean(KEY_VPN_DETECT_ENABLED, true),
        screenshotBlock = prefs.getBoolean(KEY_SCREENSHOT_BLOCK, false),
        dnsOverHttps = prefs.getBoolean(KEY_DNS_OVER_HTTPS, false),
        dnsOverHttpsProvider = prefs.getString(KEY_DNS_OVER_HTTPS_PROVIDER, "cloudflare") ?: "cloudflare",
        arpSpoofDetect = prefs.getBoolean(KEY_ARP_SPOOF_DETECT, true),
        macRandomization = prefs.getBoolean(KEY_MAC_RANDOMIZATION, false)
    )

    fun save(settings: AppSettings) {
        prefs.edit {
            putInt(KEY_DETECTION_LEVEL, settings.detectionLevel.value)
            putBoolean(KEY_AUTO_SCAN, settings.autoScanEnabled)
            putInt(KEY_AUTO_SCAN_INTERVAL, settings.autoScanInterval.value)
            putBoolean(KEY_RISK_NOTIFICATION, settings.riskNotificationEnabled)
            putInt(KEY_RISK_THRESHOLD, settings.riskThreshold)
            putBoolean(KEY_PLUGIN_L1, settings.pluginL1Enabled)
            putBoolean(KEY_PLUGIN_L2, settings.pluginL2Enabled)
            putBoolean(KEY_PLUGIN_L3, settings.pluginL3Enabled)
            putBoolean(KEY_PLUGIN_L4, settings.pluginL4Enabled)
            putBoolean(KEY_PLUGIN_L5, settings.pluginL5Enabled)
            putBoolean(KEY_PLUGIN_L6, settings.pluginL6Enabled)
            putBoolean(KEY_PLUGIN_L7, settings.pluginL7Enabled)
            putBoolean(KEY_GUARD_ENABLED, settings.guardEnabled)
            putInt(KEY_GUARD_LEVEL, settings.guardLevel.value)
            putInt(KEY_GUARD_SELF_CHECK_INTERVAL, settings.guardSelfCheckInterval)
            putBoolean(KEY_GUARD_ANTI_DEBUG, settings.guardAntiDebug)
            putBoolean(KEY_GUARD_AUTO_RECOVERY, settings.guardAutoRecovery)
            putInt(KEY_HIDE_STRATEGY, settings.hideStrategy.value)
            putBoolean(KEY_HWID_SPOOF, settings.hwidSpoofEnabled)
            putBoolean(KEY_BOOTLOADER_SPOOF, settings.bootloaderSpoofEnabled)
            putBoolean(KEY_GAME_MODE, settings.gameModeEnabled)
            putBoolean(KEY_GAME_MODE_AGGRESSIVE, settings.gameModeAggressive)
            putBoolean(KEY_SANDBOX_AUTO_CREATE, settings.sandboxAutoCreate)
            putBoolean(KEY_SANDBOX_CLEANUP, settings.sandboxCleanupOnExit)
            putInt(KEY_CURE_DEFAULT_LEVEL, settings.cureDefaultLevel)
            putBoolean(KEY_CURE_AUTO_FIX, settings.cureAutoFix)
            putInt(KEY_THEME_MODE, settings.themeMode.value)
            putInt(KEY_ACCENT_COLOR, settings.accentColor.value)
            putBoolean(KEY_ANIMATIONS, settings.animationsEnabled)
            putBoolean(KEY_LIQUID_GLASS, settings.liquidGlassEffect)
            putBoolean(KEY_EBPF_FIREWALL, settings.ebpfFirewallEnabled)
            putBoolean(KEY_FIREWALL_LOGGING, settings.firewallLogging)
            putString(KEY_UPDATE_PROXY, settings.updateProxy)
            putBoolean(KEY_NOTIFY_SCAN_COMPLETE, settings.notifyScanComplete)
            putBoolean(KEY_NOTIFY_RISK_FOUND, settings.notifyRiskFound)
            putBoolean(KEY_NOTIFY_GUARD_ALERT, settings.notifyGuardAlert)
            putBoolean(KEY_NOTIFY_CURE_RESULT, settings.notifyCureResult)
            putBoolean(KEY_NOTIFY_UPDATE_AVAILABLE, settings.notifyUpdateAvailable)
            putBoolean(KEY_TELEMETRY, settings.telemetryEnabled)
            putBoolean(KEY_CRASH_REPORTS, settings.crashReportsEnabled)
            putBoolean(KEY_AUTO_CHECK_UPDATES, settings.autoCheckUpdates)
            putInt(KEY_UPDATE_CHANNEL, settings.updateChannel.value)
            putBoolean(KEY_WIFI_ONLY_UPDATE, settings.wifiOnlyUpdate)
            putInt(KEY_SCAN_CONCURRENCY, settings.scanConcurrency)
            putInt(KEY_CACHE_MAX_AGE, settings.cacheMaxAgeMinutes)
            putBoolean(KEY_LOW_MEMORY_MODE, settings.lowMemoryMode)
            putInt(KEY_LOG_LEVEL, settings.logLevel.value)
            putInt(KEY_LOG_MAX_FILES, settings.logMaxFiles)
            putBoolean(KEY_LOG_AUTO_CLEANUP, settings.logAutoCleanup)
            putInt(KEY_SCHEDULE_TIME, settings.scheduleTime.value)
            putInt(KEY_LANGUAGE, settings.language.value)
            putBoolean(KEY_EXPERIMENTAL, settings.experimentalFeatures)
            putInt(KEY_POWER_PROFILE, settings.powerProfile.value)
            putBoolean(KEY_BATTERY_SAVER, settings.batterySaverEnabled)
            putInt(KEY_IPC_TIMEOUT, settings.ipcTimeoutSeconds)
            putInt(KEY_IPC_RETRY, settings.ipcRetryCount)
            putString(KEY_IPC_SOCKET_PATH, settings.ipcSocketPath)
            putBoolean(KEY_WATCHDOG_ENABLED, settings.watchdogEnabled)
            putInt(KEY_WATCHDOG_INTERVAL, settings.watchdogIntervalSeconds)
            putBoolean(KEY_WATCHDOG_AUTO_RESTART, settings.watchdogAutoRestart)
            putInt(KEY_OBFUSCATION_LEVEL, settings.obfuscationLevel.value)
            putBoolean(KEY_CPU_RANDOM_AFFINITY, settings.cpuRandomAffinity)
            putString(KEY_WHITELIST_APPS, settings.whitelistApps)
            putString(KEY_SCAN_EXCLUSIONS, settings.scanExclusions)
            putString(KEY_CUSTOM_KEYWORDS, settings.customDetectionKeywords)
            putString(KEY_CUSTOM_HIDE_PATHS, settings.customHidePaths)
            putBoolean(KEY_VERIFY_BOOT, settings.verifyBootEnabled)
            putBoolean(KEY_KEYSTORE_CHECK, settings.keystoreCheckEnabled)
            putBoolean(KEY_SELINUX_ENFORCE, settings.selinuxEnforceCheck)
            putString(KEY_CURRENT_PROFILE, settings.currentProfile)
            putBoolean(KEY_STEALTH_HIDE_ICON, settings.stealthHideIcon)
            putBoolean(KEY_STEALTH_HIDE_RECENT, settings.stealthHideRecent)
            putBoolean(KEY_STEALTH_HIDE_NOTIFICATION, settings.stealthHideNotification)
            putString(KEY_STEALTH_FAKE_APP_NAME, settings.stealthFakeAppName)
            putBoolean(KEY_EMERGENCY_LOCK, settings.emergencyLockDevice)
            putBoolean(KEY_EMERGENCY_WIPE, settings.emergencyWipeData)
            putBoolean(KEY_EMERGENCY_SHUTDOWN, settings.emergencyShutdown)
            putBoolean(KEY_EMERGENCY_NOTIFY, settings.emergencyNotifyContacts)
            putBoolean(KEY_PMU_SPOOF, settings.pmuSpoofEnabled)
            putBoolean(KEY_TIMING_SPOOF, settings.timingSpoofEnabled)
            putBoolean(KEY_MEMORY_SPOOF, settings.memorySpoofEnabled)
            putBoolean(KEY_SOUND_SCAN_COMPLETE, settings.soundScanComplete)
            putBoolean(KEY_SOUND_RISK_ALERT, settings.soundRiskAlert)
            putBoolean(KEY_SOUND_GUARD_ALERT, settings.soundGuardAlert)
            putBoolean(KEY_HAPTIC_FEEDBACK, settings.hapticFeedback)
            putBoolean(KEY_APP_LOCK_ENABLED, settings.appLockEnabled)
            putString(KEY_APP_LOCK_PIN, settings.appLockPin)
            putBoolean(KEY_APP_LOCK_BIOMETRIC, settings.appLockBiometric)
            putBoolean(KEY_AUTO_EXPORT, settings.autoExportEnabled)
            putString(KEY_EXPORT_FORMAT, settings.exportFormat)
            putString(KEY_CUSTOM_ENCRYPTION_KEY, settings.customEncryptionKey)
            putInt(KEY_KEY_ROTATION_HOURS, settings.keyRotationHours)
            putBoolean(KEY_NETWORK_DNS_MONITOR, settings.networkDnsMonitor)
            putBoolean(KEY_NETWORK_TRAFFIC_MONITOR, settings.networkTrafficMonitor)
            putBoolean(KEY_KERNEL_MODULE_CHECK, settings.kernelModuleCheckEnabled)
            putBoolean(KEY_AUTO_SILENCE_ON_GAME, settings.autoSilenceOnGame)
            putBoolean(KEY_QUICK_SCAN_TOGGLE, settings.quickScanToggle)
            putBoolean(KEY_FLOATING_WIDGET, settings.floatingWidget)
            putBoolean(KEY_GESTURE_SHORTCUT, settings.gestureShortcut)
            putBoolean(KEY_HOME_SCREEN_WIDGET, settings.homeScreenWidget)
            putBoolean(KEY_AUTO_BACKUP, settings.autoBackupEnabled)
            putInt(KEY_BACKUP_INTERVAL_DAYS, settings.backupIntervalDays)
            putString(KEY_BACKUP_PATH, settings.backupPath)
            putBoolean(KEY_CLOUD_SYNC, settings.cloudSyncEnabled)
            putBoolean(KEY_USB_DETECTION_ALERT, settings.usbDetectionAlert)
            putBoolean(KEY_WIFI_SECURITY_SCAN, settings.wifiSecurityScan)
            putBoolean(KEY_BLE_SCAN, settings.bleScanEnabled)
            putBoolean(KEY_UNTRUSTED_NETWORK_ALERT, settings.untrustedNetworkAlert)
            putBoolean(KEY_TASKER_PLUGIN, settings.taskerPluginEnabled)
            putString(KEY_WEBHOOK_URL, settings.webhookUrl)
            putBoolean(KEY_INTENT_TRIGGER, settings.intentTriggerEnabled)
            putBoolean(KEY_SMS_COMMANDS, settings.smsCommandsEnabled)
            putBoolean(KEY_REMOTE_PAIRING, settings.remotePairingEnabled)
            putBoolean(KEY_FCM_REMOTE_COMMANDS, settings.fcmRemoteCommands)
            putString(KEY_PAIRING_CODE, settings.pairingCode)
            putBoolean(KEY_AUTO_PAIR_TRUSTED, settings.autoPairTrusted)
            putInt(KEY_FONT_SCALE, settings.fontScale)
            putBoolean(KEY_COMPACT_MODE, settings.compactModeEnabled)
            putBoolean(KEY_STATUS_BAR_INDICATOR, settings.statusBarIndicator)
            putBoolean(KEY_FLOATING_WIDGET_ENABLED, settings.floatingWidgetEnabled)
            putString(KEY_REPORT_STORAGE_PATH, settings.reportStoragePath)
            putBoolean(KEY_CACHE_CLEANUP_ON_EXIT, settings.cacheCleanupOnExit)
            putInt(KEY_MAX_CACHE_SIZE_MB, settings.maxCacheSizeMb)
            putInt(KEY_BACKUP_RETENTION_DAYS, settings.backupRetentionDays)
            putBoolean(KEY_ADB_MONITOR, settings.adbMonitorEnabled)
            putBoolean(KEY_DEV_MODE_WARNING, settings.devModeWarning)
            putBoolean(KEY_DEBUG_LOGGING, settings.debugLoggingEnabled)
            putBoolean(KEY_MOCK_LOCATION_DETECTION, settings.mockLocationDetection)
            putBoolean(KEY_FILE_INTEGRITY_CHECK, settings.fileIntegrityCheck)
            putBoolean(KEY_INOTIFY_WATCH, settings.inotifyWatchEnabled)
            putBoolean(KEY_SYSTEM_PARTITION_MONITOR, settings.systemPartitionMonitor)
            putBoolean(KEY_CRITICAL_FILE_ALERT, settings.criticalFileAlert)
            putBoolean(KEY_SUSPICIOUS_PROCESS_DETECT, settings.suspiciousProcessDetect)
            putString(KEY_PROCESS_WHITELIST, settings.processWhitelist)
            putBoolean(KEY_BG_PROCESS_MONITOR, settings.bgProcessMonitor)
            putBoolean(KEY_PROCESS_ANOMALY_ALERT, settings.processAnomalyAlert)
            putBoolean(KEY_BATTERY_WHITELIST, settings.batteryWhitelistEnabled)
            putBoolean(KEY_DOZE_EXEMPT, settings.dozeExemptEnabled)
            putBoolean(KEY_WAKE_LOCK_CONTROL, settings.wakeLockControl)
            putBoolean(KEY_POWER_SAVE_OVERRIDE, settings.powerSaveOverride)
            putBoolean(KEY_APP_PERMISSION_MONITOR, settings.appPermissionMonitor)
            putBoolean(KEY_PERMISSION_CHANGE_ALERT, settings.permissionChangeAlert)
            putBoolean(KEY_SUSPICIOUS_PERMISSION_DETECT, settings.suspiciousPermissionDetect)
            putBoolean(KEY_RUNTIME_PERMISSION_AUDIT, settings.runtimePermissionAudit)
            putBoolean(KEY_CAMERA_ACCESS_MONITOR, settings.cameraAccessMonitor)
            putBoolean(KEY_MIC_ACCESS_MONITOR, settings.micAccessMonitor)
            putBoolean(KEY_SENSOR_ACCESS_LOG, settings.sensorAccessLog)
            putBoolean(KEY_CLIPBOARD_MONITOR, settings.clipboardMonitorEnabled)
            putBoolean(KEY_AUDIT_TRAIL, settings.auditTrailEnabled)
            putBoolean(KEY_COMPLIANCE_MODE, settings.complianceModeEnabled)
            putInt(KEY_EVENT_RETENTION_DAYS, settings.eventRetentionDays)
            putBoolean(KEY_AUDIT_EXPORT, settings.auditExportEnabled)
            putBoolean(KEY_COMMUNITY_FORUM, settings.communityForum)
            putBoolean(KEY_DISCORD_LINK, settings.discordLink)
            putBoolean(KEY_TELEGRAM_BOT, settings.telegramBotEnabled)
            putBoolean(KEY_GITHUB_INTEGRATION, settings.githubIntegration)
            putBoolean(KEY_BETA_FEATURE_FLAGS, settings.betaFeatureFlags)
            putBoolean(KEY_UNRELEASED_MOD, settings.unreleasedModEnabled)
            putBoolean(KEY_ML_DETECTION, settings.mlDetectionEnabled)
            putBoolean(KEY_EARLY_ACCESS, settings.earlyAccessProgram)
            putString(KEY_SU_BINARY_PATH, settings.suBinaryPath)
            putInt(KEY_ROOT_ACCESS_TIMEOUT, settings.rootAccessTimeout)
            putBoolean(KEY_ROOT_PERMISSION_AUDIT, settings.rootPermissionAudit)
            putBoolean(KEY_MULTI_USER_SUPPORT, settings.multiUserSupport)
            putString(KEY_SELINUX_CONTEXT, settings.selinuxContext)
            putBoolean(KEY_SELINUX_POLICY_CHECK, settings.selinuxPolicyCheck)
            putBoolean(KEY_SELINUX_MODE_TOGGLE, settings.selinuxModeToggle)
            putBoolean(KEY_CONTEXT_LABEL_VERIFY, settings.contextLabelVerify)
            putBoolean(KEY_MOUNT_NS_ISOLATION, settings.mountNamespaceIsolation)
            putBoolean(KEY_PID_NAMESPACE, settings.pidNamespace)
            putBoolean(KEY_NETWORK_NAMESPACE, settings.networkNamespace)
            putBoolean(KEY_IPC_NAMESPACE, settings.ipcNamespace)
            putString(KEY_MODULE_LOAD_ORDER, settings.moduleLoadOrder)
            putBoolean(KEY_MODULE_COMPAT_CHECK, settings.moduleCompatCheck)
            putString(KEY_MODULE_UPDATE_POLICY, settings.moduleUpdatePolicy)
            putString(KEY_MODULE_BLACKLIST, settings.moduleBlacklist)
            putString(KEY_REPORT_DETAIL_LEVEL, settings.reportDetailLevel)
            putString(KEY_REPORT_FORMAT_DETAILED, settings.reportFormatDetailed)
            putBoolean(KEY_AUTO_REPORT_SHARE, settings.autoReportShare)
            putBoolean(KEY_REPORT_ENCRYPTION, settings.reportEncryption)
            putBoolean(KEY_SECURE_KEYBOARD, settings.secureKeyboard)
            putBoolean(KEY_INPUT_OBFUSCATION, settings.inputObfuscation)
            putBoolean(KEY_OVERLAY_PROTECTION, settings.overlayProtection)
            putBoolean(KEY_KEYLOGGING_DETECT, settings.keyloggingDetect)
            putBoolean(KEY_WIFI_HOTSPOT_DETECT, settings.wifiHotspotDetect)
            putBoolean(KEY_BLUETOOTH_TETHER_ALERT, settings.bluetoothTetherAlert)
            putBoolean(KEY_NFC_DETECTION, settings.nfcDetection)
            putBoolean(KEY_AIRPLANE_MODE_BEHAVIOR, settings.airplaneModeBehavior)
            putString(KEY_EMERGENCY_SMS_TEMPLATE, settings.emergencySmsTemplate)
            putBoolean(KEY_EMERGENCY_CALL, settings.emergencyCall)
            putBoolean(KEY_LOCATION_SHARING, settings.locationSharing)
            putBoolean(KEY_HEARTBEAT_PING, settings.heartbeatPing)
            putBoolean(KEY_PLAY_INTEGRITY, settings.playIntegrityCheck)
            putBoolean(KEY_KEY_ATTESTATION, settings.keyAttestationEnabled)
            putBoolean(KEY_CTS_PROFILE, settings.ctsProfileMatch)
            putBoolean(KEY_BASIC_INTEGRITY, settings.basicIntegrity)
            putBoolean(KEY_ALWAYS_ON_VPN, settings.alwaysOnVpn)
            putBoolean(KEY_VPN_KILL_SWITCH, settings.vpnKillSwitch)
            putBoolean(KEY_DNS_LEAK_PROTECTION, settings.dnsLeakProtection)
            putBoolean(KEY_SPLIT_TUNNELING, settings.splitTunneling)
            putString(KEY_VPN_PROXY_CONFIG, settings.vpnProxyConfig)
            putBoolean(KEY_CERT_PINNING, settings.certPinningEnabled)
            putString(KEY_SSL_VERIFICATION_LEVEL, settings.sslVerificationLevel)
            putString(KEY_TRUSTED_CA_LIST, settings.trustedCaList)
            putBoolean(KEY_CERT_TRANSPARENCY, settings.certTransparency)
            putBoolean(KEY_MAGISK_VERSION_CHECK, settings.magiskVersionCheck)
            putBoolean(KEY_MAGISK_MODULE_SCAN, settings.magiskModuleScan)
            putBoolean(KEY_ZYGISK_DETECT, settings.zygiskDetect)
            putString(KEY_MAGISK_HIDE_LIST, settings.magiskHideList)
            putBoolean(KEY_XPOSED_DETECT, settings.xposedDetectEnabled)
            putBoolean(KEY_XPOSED_MODULE_SCAN, settings.xposedModuleScan)
            putBoolean(KEY_LSPOSED_DETECT, settings.lsposedDetectEnabled)
            putBoolean(KEY_RIRU_DETECT, settings.riruDetectEnabled)
            putString(KEY_CUSTOM_INIT_SCRIPTS, settings.customInitScripts)
            putString(KEY_POST_BOOT_SCRIPTS, settings.postBootScripts)
            putBoolean(KEY_SCRIPT_SCHEDULE, settings.scriptRunOnSchedule)
            putBoolean(KEY_SCRIPT_LOG_OUTPUT, settings.scriptLogOutput)
            putBoolean(KEY_HARDWARE_ATTESTATION, settings.hardwareAttestation)
            putBoolean(KEY_TEE_CHECK, settings.teeCheckEnabled)
            putBoolean(KEY_SECURE_ELEMENT, settings.secureElementCheck)
            putBoolean(KEY_FINGERPRINT_HARDWARE, settings.fingerprintHardwareCheck)
            putBoolean(KEY_DM_VERITY, settings.dmVerityCheck)
            putBoolean(KEY_AVB, settings.avbCheck)
            putBoolean(KEY_SYSTEM_PARTITION_RO, settings.systemPartitionRo)
            putBoolean(KEY_VBMETA, settings.vbmetaCheck)
            putBoolean(KEY_SCREEN_RECORD_DETECT, settings.screenRecordDetect)
            putBoolean(KEY_VPN_DETECT_ENABLED, settings.vpnDetectEnabled)
            putBoolean(KEY_SCREENSHOT_BLOCK, settings.screenshotBlock)
            putBoolean(KEY_DNS_OVER_HTTPS, settings.dnsOverHttps)
            putString(KEY_DNS_OVER_HTTPS_PROVIDER, settings.dnsOverHttpsProvider)
            putBoolean(KEY_ARP_SPOOF_DETECT, settings.arpSpoofDetect)
            putBoolean(KEY_MAC_RANDOMIZATION, settings.macRandomization)
        }
    }

    companion object {
        private const val KEY_DETECTION_LEVEL = "detection_level"
        private const val KEY_AUTO_SCAN = "auto_scan"
        private const val KEY_AUTO_SCAN_INTERVAL = "auto_scan_interval"
        private const val KEY_RISK_NOTIFICATION = "risk_notification"
        private const val KEY_RISK_THRESHOLD = "risk_threshold"
        private const val KEY_PLUGIN_L1 = "plugin_l1"
        private const val KEY_PLUGIN_L2 = "plugin_l2"
        private const val KEY_PLUGIN_L3 = "plugin_l3"
        private const val KEY_PLUGIN_L4 = "plugin_l4"
        private const val KEY_PLUGIN_L5 = "plugin_l5"
        private const val KEY_PLUGIN_L6 = "plugin_l6"
        private const val KEY_PLUGIN_L7 = "plugin_l7"
        private const val KEY_GUARD_ENABLED = "guard_enabled"
        private const val KEY_GUARD_LEVEL = "guard_level"
        private const val KEY_GUARD_SELF_CHECK_INTERVAL = "guard_self_check_interval"
        private const val KEY_GUARD_ANTI_DEBUG = "guard_anti_debug"
        private const val KEY_GUARD_AUTO_RECOVERY = "guard_auto_recovery"
        private const val KEY_HIDE_STRATEGY = "hide_strategy"
        private const val KEY_HWID_SPOOF = "hwid_spoof"
        private const val KEY_BOOTLOADER_SPOOF = "bootloader_spoof"
        private const val KEY_GAME_MODE = "game_mode"
        private const val KEY_GAME_MODE_AGGRESSIVE = "game_mode_aggressive"
        private const val KEY_SANDBOX_AUTO_CREATE = "sandbox_auto_create"
        private const val KEY_SANDBOX_CLEANUP = "sandbox_cleanup"
        private const val KEY_CURE_DEFAULT_LEVEL = "cure_default_level"
        private const val KEY_CURE_AUTO_FIX = "cure_auto_fix"
        private const val KEY_THEME_MODE = "theme_mode"
        private const val KEY_ACCENT_COLOR = "accent_color"
        private const val KEY_ANIMATIONS = "animations"
        private const val KEY_LIQUID_GLASS = "liquid_glass"
        private const val KEY_EBPF_FIREWALL = "ebpf_firewall"
        private const val KEY_FIREWALL_LOGGING = "firewall_logging"
        private const val KEY_UPDATE_PROXY = "update_proxy"
        private const val KEY_NOTIFY_SCAN_COMPLETE = "notify_scan_complete"
        private const val KEY_NOTIFY_RISK_FOUND = "notify_risk_found"
        private const val KEY_NOTIFY_GUARD_ALERT = "notify_guard_alert"
        private const val KEY_NOTIFY_CURE_RESULT = "notify_cure_result"
        private const val KEY_NOTIFY_UPDATE_AVAILABLE = "notify_update_available"
        private const val KEY_TELEMETRY = "telemetry"
        private const val KEY_CRASH_REPORTS = "crash_reports"
        private const val KEY_AUTO_CHECK_UPDATES = "auto_check_updates"
        private const val KEY_UPDATE_CHANNEL = "update_channel"
        private const val KEY_WIFI_ONLY_UPDATE = "wifi_only_update"
        private const val KEY_SCAN_CONCURRENCY = "scan_concurrency"
        private const val KEY_CACHE_MAX_AGE = "cache_max_age"
        private const val KEY_LOW_MEMORY_MODE = "low_memory_mode"
        private const val KEY_LOG_LEVEL = "log_level"
        private const val KEY_LOG_MAX_FILES = "log_max_files"
        private const val KEY_LOG_AUTO_CLEANUP = "log_auto_cleanup"
        private const val KEY_SCHEDULE_TIME = "schedule_time"
        private const val KEY_LANGUAGE = "language"
        private const val KEY_EXPERIMENTAL = "experimental"
        private const val KEY_POWER_PROFILE = "power_profile"
        private const val KEY_BATTERY_SAVER = "battery_saver"
        private const val KEY_IPC_TIMEOUT = "ipc_timeout"
        private const val KEY_IPC_RETRY = "ipc_retry"
        private const val KEY_IPC_SOCKET_PATH = "ipc_socket_path"
        private const val KEY_WATCHDOG_ENABLED = "watchdog_enabled"
        private const val KEY_WATCHDOG_INTERVAL = "watchdog_interval"
        private const val KEY_WATCHDOG_AUTO_RESTART = "watchdog_auto_restart"
        private const val KEY_OBFUSCATION_LEVEL = "obfuscation_level"
        private const val KEY_CPU_RANDOM_AFFINITY = "cpu_random_affinity"
        private const val KEY_WHITELIST_APPS = "whitelist_apps"
        private const val KEY_SCAN_EXCLUSIONS = "scan_exclusions"
        private const val KEY_CUSTOM_KEYWORDS = "custom_keywords"
        private const val KEY_CUSTOM_HIDE_PATHS = "custom_hide_paths"
        private const val KEY_VERIFY_BOOT = "verify_boot"
        private const val KEY_KEYSTORE_CHECK = "keystore_check"
        private const val KEY_SELINUX_ENFORCE = "selinux_enforce"
        private const val KEY_CURRENT_PROFILE = "current_profile"
        private const val KEY_STEALTH_HIDE_ICON = "stealth_hide_icon"
        private const val KEY_STEALTH_HIDE_RECENT = "stealth_hide_recent"
        private const val KEY_STEALTH_HIDE_NOTIFICATION = "stealth_hide_notification"
        private const val KEY_STEALTH_FAKE_APP_NAME = "stealth_fake_app_name"
        private const val KEY_EMERGENCY_LOCK = "emergency_lock"
        private const val KEY_EMERGENCY_WIPE = "emergency_wipe"
        private const val KEY_EMERGENCY_SHUTDOWN = "emergency_shutdown"
        private const val KEY_EMERGENCY_NOTIFY = "emergency_notify"
        private const val KEY_PMU_SPOOF = "pmu_spoof"
        private const val KEY_TIMING_SPOOF = "timing_spoof"
        private const val KEY_MEMORY_SPOOF = "memory_spoof"
        private const val KEY_SOUND_SCAN_COMPLETE = "sound_scan_complete"
        private const val KEY_SOUND_RISK_ALERT = "sound_risk_alert"
        private const val KEY_SOUND_GUARD_ALERT = "sound_guard_alert"
        private const val KEY_HAPTIC_FEEDBACK = "haptic_feedback"
        private const val KEY_APP_LOCK_ENABLED = "app_lock_enabled"
        private const val KEY_APP_LOCK_PIN = "app_lock_pin"
        private const val KEY_APP_LOCK_BIOMETRIC = "app_lock_biometric"
        private const val KEY_AUTO_EXPORT = "auto_export"
        private const val KEY_EXPORT_FORMAT = "export_format"
        private const val KEY_CUSTOM_ENCRYPTION_KEY = "custom_encryption_key"
        private const val KEY_KEY_ROTATION_HOURS = "key_rotation_hours"
        private const val KEY_NETWORK_DNS_MONITOR = "network_dns_monitor"
        private const val KEY_NETWORK_TRAFFIC_MONITOR = "network_traffic_monitor"
        private const val KEY_KERNEL_MODULE_CHECK = "kernel_module_check"
        private const val KEY_AUTO_SILENCE_ON_GAME = "auto_silence_on_game"
        private const val KEY_QUICK_SCAN_TOGGLE = "quick_scan_toggle"
        private const val KEY_FLOATING_WIDGET = "floating_widget"
        private const val KEY_GESTURE_SHORTCUT = "gesture_shortcut"
        private const val KEY_HOME_SCREEN_WIDGET = "home_screen_widget"
        private const val KEY_AUTO_BACKUP = "auto_backup"
        private const val KEY_BACKUP_INTERVAL_DAYS = "backup_interval_days"
        private const val KEY_BACKUP_PATH = "backup_path"
        private const val KEY_CLOUD_SYNC = "cloud_sync"
        private const val KEY_USB_DETECTION_ALERT = "usb_detection_alert"
        private const val KEY_WIFI_SECURITY_SCAN = "wifi_security_scan"
        private const val KEY_BLE_SCAN = "ble_scan"
        private const val KEY_UNTRUSTED_NETWORK_ALERT = "untrusted_network_alert"
        private const val KEY_TASKER_PLUGIN = "tasker_plugin"
        private const val KEY_WEBHOOK_URL = "webhook_url"
        private const val KEY_INTENT_TRIGGER = "intent_trigger"
        private const val KEY_SMS_COMMANDS = "sms_commands"
        private const val KEY_REMOTE_PAIRING = "remote_pairing"
        private const val KEY_FCM_REMOTE_COMMANDS = "fcm_remote_commands"
        private const val KEY_PAIRING_CODE = "pairing_code"
        private const val KEY_AUTO_PAIR_TRUSTED = "auto_pair_trusted"
        private const val KEY_FONT_SCALE = "font_scale"
        private const val KEY_COMPACT_MODE = "compact_mode"
        private const val KEY_STATUS_BAR_INDICATOR = "status_bar_indicator"
        private const val KEY_FLOATING_WIDGET_ENABLED = "floating_widget_enabled"
        private const val KEY_REPORT_STORAGE_PATH = "report_storage_path"
        private const val KEY_CACHE_CLEANUP_ON_EXIT = "cache_cleanup_on_exit"
        private const val KEY_MAX_CACHE_SIZE_MB = "max_cache_size_mb"
        private const val KEY_BACKUP_RETENTION_DAYS = "backup_retention_days"
        private const val KEY_ADB_MONITOR = "adb_monitor"
        private const val KEY_DEV_MODE_WARNING = "dev_mode_warning"
        private const val KEY_DEBUG_LOGGING = "debug_logging"
        private const val KEY_MOCK_LOCATION_DETECTION = "mock_location_detection"
        private const val KEY_FILE_INTEGRITY_CHECK = "file_integrity_check"
        private const val KEY_INOTIFY_WATCH = "inotify_watch"
        private const val KEY_SYSTEM_PARTITION_MONITOR = "system_partition_monitor"
        private const val KEY_CRITICAL_FILE_ALERT = "critical_file_alert"
        private const val KEY_SUSPICIOUS_PROCESS_DETECT = "suspicious_process_detect"
        private const val KEY_PROCESS_WHITELIST = "process_whitelist"
        private const val KEY_BG_PROCESS_MONITOR = "bg_process_monitor"
        private const val KEY_PROCESS_ANOMALY_ALERT = "process_anomaly_alert"
        private const val KEY_BATTERY_WHITELIST = "battery_whitelist"
        private const val KEY_DOZE_EXEMPT = "doze_exempt"
        private const val KEY_WAKE_LOCK_CONTROL = "wake_lock_control"
        private const val KEY_POWER_SAVE_OVERRIDE = "power_save_override"
        private const val KEY_APP_PERMISSION_MONITOR = "app_permission_monitor"
        private const val KEY_PERMISSION_CHANGE_ALERT = "permission_change_alert"
        private const val KEY_SUSPICIOUS_PERMISSION_DETECT = "suspicious_permission_detect"
        private const val KEY_RUNTIME_PERMISSION_AUDIT = "runtime_permission_audit"
        private const val KEY_CAMERA_ACCESS_MONITOR = "camera_access_monitor"
        private const val KEY_MIC_ACCESS_MONITOR = "mic_access_monitor"
        private const val KEY_SENSOR_ACCESS_LOG = "sensor_access_log"
        private const val KEY_CLIPBOARD_MONITOR = "clipboard_monitor"
        private const val KEY_AUDIT_TRAIL = "audit_trail"
        private const val KEY_COMPLIANCE_MODE = "compliance_mode"
        private const val KEY_EVENT_RETENTION_DAYS = "event_retention_days"
        private const val KEY_AUDIT_EXPORT = "audit_export"
        private const val KEY_COMMUNITY_FORUM = "community_forum"
        private const val KEY_DISCORD_LINK = "discord_link"
        private const val KEY_TELEGRAM_BOT = "telegram_bot"
        private const val KEY_GITHUB_INTEGRATION = "github_integration"
        private const val KEY_BETA_FEATURE_FLAGS = "beta_feature_flags"
        private const val KEY_UNRELEASED_MOD = "unreleased_mod"
        private const val KEY_ML_DETECTION = "ml_detection"
        private const val KEY_EARLY_ACCESS = "early_access"
        private const val KEY_SU_BINARY_PATH = "su_binary_path"
        private const val KEY_ROOT_ACCESS_TIMEOUT = "root_access_timeout"
        private const val KEY_ROOT_PERMISSION_AUDIT = "root_permission_audit"
        private const val KEY_MULTI_USER_SUPPORT = "multi_user_support"
        private const val KEY_SELINUX_CONTEXT = "selinux_context"
        private const val KEY_SELINUX_POLICY_CHECK = "selinux_policy_check"
        private const val KEY_SELINUX_MODE_TOGGLE = "selinux_mode_toggle"
        private const val KEY_CONTEXT_LABEL_VERIFY = "context_label_verify"
        private const val KEY_MOUNT_NS_ISOLATION = "mount_ns_isolation"
        private const val KEY_PID_NAMESPACE = "pid_namespace"
        private const val KEY_NETWORK_NAMESPACE = "network_namespace"
        private const val KEY_IPC_NAMESPACE = "ipc_namespace"
        private const val KEY_MODULE_LOAD_ORDER = "module_load_order"
        private const val KEY_MODULE_COMPAT_CHECK = "module_compat_check"
        private const val KEY_MODULE_UPDATE_POLICY = "module_update_policy"
        private const val KEY_MODULE_BLACKLIST = "module_blacklist"
        private const val KEY_REPORT_DETAIL_LEVEL = "report_detail_level"
        private const val KEY_REPORT_FORMAT_DETAILED = "report_format_detailed"
        private const val KEY_AUTO_REPORT_SHARE = "auto_report_share"
        private const val KEY_REPORT_ENCRYPTION = "report_encryption"
        private const val KEY_SECURE_KEYBOARD = "secure_keyboard"
        private const val KEY_INPUT_OBFUSCATION = "input_obfuscation"
        private const val KEY_OVERLAY_PROTECTION = "overlay_protection"
        private const val KEY_KEYLOGGING_DETECT = "keylogging_detect"
        private const val KEY_WIFI_HOTSPOT_DETECT = "wifi_hotspot_detect"
        private const val KEY_BLUETOOTH_TETHER_ALERT = "bluetooth_tether_alert"
        private const val KEY_NFC_DETECTION = "nfc_detection"
        private const val KEY_AIRPLANE_MODE_BEHAVIOR = "airplane_mode_behavior"
        private const val KEY_EMERGENCY_SMS_TEMPLATE = "emergency_sms_template"
        private const val KEY_EMERGENCY_CALL = "emergency_call"
        private const val KEY_LOCATION_SHARING = "location_sharing"
        private const val KEY_HEARTBEAT_PING = "heartbeat_ping"
        private const val KEY_PLAY_INTEGRITY = "play_integrity"
        private const val KEY_KEY_ATTESTATION = "key_attestation"
        private const val KEY_CTS_PROFILE = "cts_profile"
        private const val KEY_BASIC_INTEGRITY = "basic_integrity"
        private const val KEY_ALWAYS_ON_VPN = "always_on_vpn"
        private const val KEY_VPN_KILL_SWITCH = "vpn_kill_switch"
        private const val KEY_DNS_LEAK_PROTECTION = "dns_leak_protection"
        private const val KEY_SPLIT_TUNNELING = "split_tunneling"
        private const val KEY_VPN_PROXY_CONFIG = "vpn_proxy_config"
        private const val KEY_CERT_PINNING = "cert_pinning"
        private const val KEY_SSL_VERIFICATION_LEVEL = "ssl_verification_level"
        private const val KEY_TRUSTED_CA_LIST = "trusted_ca_list"
        private const val KEY_CERT_TRANSPARENCY = "cert_transparency"
        private const val KEY_MAGISK_VERSION_CHECK = "magisk_version_check"
        private const val KEY_MAGISK_MODULE_SCAN = "magisk_module_scan"
        private const val KEY_ZYGISK_DETECT = "zygisk_detect"
        private const val KEY_MAGISK_HIDE_LIST = "magisk_hide_list"
        private const val KEY_XPOSED_DETECT = "xposed_detect"
        private const val KEY_XPOSED_MODULE_SCAN = "xposed_module_scan"
        private const val KEY_LSPOSED_DETECT = "lsposed_detect"
        private const val KEY_RIRU_DETECT = "riru_detect"
        private const val KEY_CUSTOM_INIT_SCRIPTS = "custom_init_scripts"
        private const val KEY_POST_BOOT_SCRIPTS = "post_boot_scripts"
        private const val KEY_SCRIPT_SCHEDULE = "script_schedule"
        private const val KEY_SCRIPT_LOG_OUTPUT = "script_log_output"
        private const val KEY_HARDWARE_ATTESTATION = "hardware_attestation"
        private const val KEY_TEE_CHECK = "tee_check"
        private const val KEY_SECURE_ELEMENT = "secure_element"
        private const val KEY_FINGERPRINT_HARDWARE = "fingerprint_hardware"
        private const val KEY_DM_VERITY = "dm_verity"
        private const val KEY_AVB = "avb"
        private const val KEY_SYSTEM_PARTITION_RO = "system_partition_ro"
        private const val KEY_VBMETA = "vbmeta"
        // Privacy Protection
        private const val KEY_SCREEN_RECORD_DETECT = "screen_record_detect"
        private const val KEY_VPN_DETECT_ENABLED = "vpn_detect_enabled"
        private const val KEY_SCREENSHOT_BLOCK = "screenshot_block"
        // Network Security
        private const val KEY_DNS_OVER_HTTPS = "dns_over_https"
        private const val KEY_DNS_OVER_HTTPS_PROVIDER = "dns_over_https_provider"
        private const val KEY_ARP_SPOOF_DETECT = "arp_spoof_detect"
        private const val KEY_MAC_RANDOMIZATION = "mac_randomization"
    }
}
