@file:OptIn(ExperimentalMaterial3Api::class)

package com.apex.root.ui.compose.screens

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.apex.root.data.*
import com.apex.root.ui.compose.*
import com.apex.root.viewmodel.SettingsViewModel
import com.apex.root.viewmodel.trusted.ApexViewModel

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SettingsScreen(
    viewModel: SettingsViewModel = viewModel(),
    apexViewModel: ApexViewModel? = null,
    onNavigateToLogs: (() -> Unit)? = null,
    onBack: (() -> Unit)? = null
) {
    val settings by viewModel.settings.collectAsState()
    var showResetDialog by remember { mutableStateOf(false) }
    val sheetState = rememberModalBottomSheetState(skipPartiallyExpanded = true)
    val snackbarHostState = remember { SnackbarHostState() }
    val scrollBehavior = TopAppBarDefaults.exitUntilCollapsedScrollBehavior()

    LaunchedEffect(Unit) {
        apexViewModel?.snackbarChannel?.collect { event ->
            snackbarHostState.currentSnackbarData?.dismiss()
            snackbarHostState.showSnackbar(event.message)
        }
    }

    if (showResetDialog) {
        GlassModalBottomSheet(
            sheetState = sheetState,
            title = "重置所有设置",
            message = "这将恢复所有设置为默认值，此操作不可撤销。",
            confirmLabel = "确认重置",
            onDismiss = { showResetDialog = false },
            onConfirm = {
                viewModel.resetToDefaults()
                showResetDialog = false
                apexViewModel?.triggerReset()
            }
        )
    }

    Scaffold(
        modifier = Modifier.nestedScroll(scrollBehavior.nestedScrollConnection),
        containerColor = Color.Transparent,
        snackbarHost = {
            SnackbarHost(hostState = snackbarHostState) { data ->
                GlassCustomSnackbar(message = data.visuals.message, type = SnackbarType.SUCCESS)
            }
        },
        topBar = {
            CollapsibleGlassTopBar(
                title = "设置",
                collapsedFraction = scrollBehavior.state.collapsedFraction,
                navigationIcon = {
                    Icon(Icons.Default.ArrowBack, "", modifier = Modifier.clickable { onBack?.invoke() })
                }
            )
        }
    ) { padding ->
        LiquidGlassContainer(fluidColorsDark = PageFluidColors.settings, fluidColorsLight = PageFluidColors.settingsLight) {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
                .verticalScroll(rememberScrollState()),
            verticalArrangement = Arrangement.spacedBy(0.dp)
        ) {
            Spacer(Modifier.height(8.dp))

            DetectionSettingsGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            GuardSettingsGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            HideSpoofSettingsGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            GameModeSettingsGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            SandboxSettingsGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            CureSettingsGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            AppearanceSettingsGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            NetworkFirewallGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            NetworkSecurityGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            NotificationsDetailGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            UpdatesGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            PrivacyGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            PrivacyProtectionGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            PerformanceGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            LoggingGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            SchedulingGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            AdvancedGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            PowerSettingsGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            IpcSettingsGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            WatchdogGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            AntiDetectionGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            WhitelistGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            HardeningGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            StealthGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            EmergencyGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            SideChannelGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            SoundHapticGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            AppLockGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            ExportGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            CryptoGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            NetworkMonitorGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            KernelGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            QuickActionsGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            BackupRestoreGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            ConnectivityGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            AutomationGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            RemoteControlGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            UserInterfaceGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            StorageGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            DeveloperGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            FileMonitorGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            ProcessGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            BatteryOptimizationGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            PermissionGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            SensorGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            AuditGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            CommunityGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            ExperimentalGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            RootManagementGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            SelinuxGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            NamespaceGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            ModuleGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            ReportGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            SecureInputGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            WirelessGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            EmergencyCommGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            IntegrityGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            VpnGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            CertGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            MagiskGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            XposedGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            ScriptGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            HardwareGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            SystemIntegrityGroup(settings, viewModel)
            Spacer(Modifier.height(24.dp))

            AboutGroup(apexViewModel, onNavigateToLogs)
            Spacer(Modifier.height(40.dp))
        }
    }
    }
}

@Composable
private fun DetectionSettingsGroup(settings: AppSettings, vm: SettingsViewModel) {
    var expanded by remember { mutableStateOf(false) }

    GlassSettingsGroup(title = "安全检测") {
        GlassSettingsItem(
            label = "检测级别",
            subtitle = settings.detectionLevel.label,
            icon = Icons.Default.Security,
            accentColor = AccentPurple,
            onClick = { expanded = !expanded }
        )
        if (expanded) {
            DetectionLevelSelector(settings.detectionLevel) { vm.updateDetectionLevel(it) }
        }
        GlassSettingsItem(
            label = "自动扫描",
            subtitle = when {
                !settings.autoScanEnabled -> "已关闭"
                else -> settings.autoScanInterval.label
            },
            icon = Icons.Default.Visibility,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.autoScanEnabled,
                    onCheckedChange = { vm.updateAutoScan(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        if (settings.autoScanEnabled) {
            AutoScanIntervalSelector(settings.autoScanInterval) { vm.updateAutoScanInterval(it) }
        }
        GlassSettingsItem(
            label = "风险通知",
            subtitle = if (settings.riskNotificationEnabled) "已开启" else "已关闭",
            icon = Icons.Default.Notifications,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.riskNotificationEnabled,
                    onCheckedChange = { vm.updateRiskNotification(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "风险阈值",
            subtitle = "${settings.riskThreshold} 分以上告警",
            icon = Icons.Default.Speed,
            accentColor = AccentGold
        )
        RiskThresholdSlider(settings.riskThreshold) { vm.updateRiskThreshold(it) }
        GlassSettingsItem(
            label = "检测插件",
            subtitle = "启用/禁用各层检测插件",
            icon = Icons.Default.Extension,
            accentColor = AccentPurple
        )
        PluginToggles(settings, vm)
    }
}

@Composable
private fun DetectionLevelSelector(current: DetectionLevel, onSelect: (DetectionLevel) -> Unit) {
    // 优化：从 FilterChip 改为 Slider 滑动开关，符合用户要求
    // 4 个级别：QUICK(0) / STANDARD(1) / DEEP(2) / FORENSIC(3)
    // steps = 2 表示在 0..3 之间有 2 个内部停靠点（即 1 和 2），共 4 个位置
    Column(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 8.dp)
    ) {
        // 当前级别显示
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                current.label,
                fontSize = 14.sp,
                fontWeight = FontWeight.SemiBold,
                color = AccentPurple
            )
            Text(
                "级别 ${current.value + 1} / ${DetectionLevel.entries.size}",
                fontSize = 11.sp,
                color = TextTertiary
            )
        }
        Spacer(Modifier.height(4.dp))
        // Slider 主体
        Row(
            modifier = Modifier.fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text("快速", fontSize = 10.sp, color = TextTertiary)
            Slider(
                value = current.value.toFloat(),
                onValueChange = { value ->
                    val newIndex = value.toInt()
                    DetectionLevel.fromValue(newIndex)?.let { onSelect(it) }
                },
                valueRange = 0f..(DetectionLevel.entries.size - 1).toFloat(),
                steps = DetectionLevel.entries.size - 2,  // N 个停靠点需要 N-2 个 steps
                modifier = Modifier.weight(1f).padding(horizontal = 8.dp),
                colors = SliderDefaults.colors(
                    thumbColor = AccentPurple,
                    activeTrackColor = AccentPurple,
                    inactiveTrackColor = AccentPurple.copy(alpha = 0.15f)
                )
            )
            Text("取证", fontSize = 10.sp, color = TextTertiary)
        }
        // 级别标签
        Row(
            modifier = Modifier.fillMaxWidth().padding(horizontal = 4.dp),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            DetectionLevel.entries.forEach { level ->
                Text(
                    level.name.first().toString(),
                    fontSize = 9.sp,
                    color = if (level == current) AccentPurple else TextTertiary
                )
            }
        }
    }
}

@Composable
private fun AutoScanIntervalSelector(current: AutoScanInterval, onSelect: (AutoScanInterval) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
        horizontalArrangement = Arrangement.spacedBy(6.dp)
    ) {
        AutoScanInterval.entries.forEach { interval ->
            FilterChip(
                selected = interval == current,
                onClick = { onSelect(interval) },
                label = { Text(interval.label, fontSize = 9.sp) },
                colors = FilterChipDefaults.filterChipColors(
                    selectedContainerColor = AccentBlue.copy(alpha = 0.2f),
                    selectedLabelColor = AccentBlue
                ),
                shape = RoundedCornerShape(10.dp)
            )
        }
    }
}

@Composable
private fun RiskThresholdSlider(threshold: Int, onChange: (Int) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text("低", fontSize = 10.sp, color = TextTertiary)
        Slider(
            value = threshold.toFloat(),
            onValueChange = { onChange(it.toInt()) },
            valueRange = 0f..100f,
            steps = 19,
            modifier = Modifier.weight(1f).padding(horizontal = 8.dp),
            colors = SliderDefaults.colors(
                thumbColor = AccentGold,
                activeTrackColor = AccentGold,
                inactiveTrackColor = AccentGold.copy(alpha = 0.15f)
            )
        )
        Text("高", fontSize = 10.sp, color = TextTertiary)
    }
}

@Composable
private fun PluginToggles(settings: AppSettings, vm: SettingsViewModel) {
    val plugins = listOf(
        1 to "L1 属性裸读", 2 to "L2 ART 注入", 3 to "L3 进程/内存",
        4 to "L4 挂载对比", 5 to "L5 侧信道", 6 to "L6 内核探针",
        7 to "L7 TEE 旁路"
    )
    Column(modifier = Modifier.padding(horizontal = 20.dp)) {
        plugins.forEach { (index, name) ->
            val enabled = when (index) {
                1 -> settings.pluginL1Enabled; 2 -> settings.pluginL2Enabled
                3 -> settings.pluginL3Enabled; 4 -> settings.pluginL4Enabled
                5 -> settings.pluginL5Enabled; 6 -> settings.pluginL6Enabled
                7 -> settings.pluginL7Enabled; else -> true
            }
            Row(
                modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(name, fontSize = 12.sp, color = TextSecondary, modifier = Modifier.weight(1f))
                Switch(
                    checked = enabled,
                    onCheckedChange = { vm.updatePluginEnabled(index, it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f),
                        uncheckedThumbColor = TextTertiary,
                        uncheckedTrackColor = TextTertiary.copy(alpha = 0.15f)
                    )
                )
            }
        }
    }
}

@Composable
private fun GuardSettingsGroup(settings: AppSettings, vm: SettingsViewModel) {
    var expanded by remember { mutableStateOf(false) }

    GlassSettingsGroup(title = "防护设置") {
        GlassSettingsItem(
            label = "APEX-Guard",
            subtitle = if (settings.guardEnabled) "已启用" else "已禁用",
            icon = Icons.Default.Shield,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.guardEnabled,
                    onCheckedChange = { vm.updateGuardEnabled(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            },
            onClick = { expanded = !expanded }
        )
        if (expanded && settings.guardEnabled) {
            GuardLevelSelector(settings.guardLevel) { vm.updateGuardLevel(it) }
            SelfCheckIntervalSlider(settings.guardSelfCheckInterval) { vm.updateGuardSelfCheckInterval(it) }
            GlassSettingsItem(
                label = "反调试保护",
                subtitle = if (settings.guardAntiDebug) "已开启" else "已关闭",
                icon = Icons.Default.RemoveRedEye,
                accentColor = ErrorRed,
                trailing = {
                    Switch(
                        checked = settings.guardAntiDebug,
                        onCheckedChange = { vm.updateGuardAntiDebug(it) },
                        colors = SwitchDefaults.colors(
                            checkedThumbColor = AccentPurple,
                            checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                        )
                    )
                }
            )
            GlassSettingsItem(
                label = "自动恢复",
                subtitle = if (settings.guardAutoRecovery) "检测到篡改自动恢复" else "仅告警",
                icon = Icons.Default.Restore,
                accentColor = AccentGold,
                trailing = {
                    Switch(
                        checked = settings.guardAutoRecovery,
                        onCheckedChange = { vm.updateGuardAutoRecovery(it) },
                        colors = SwitchDefaults.colors(
                            checkedThumbColor = AccentPurple,
                            checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                        )
                    )
                }
            )
        }
    }
}

@Composable
private fun GuardLevelSelector(current: GuardLevel, onSelect: (GuardLevel) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        GuardLevel.entries.forEach { level ->
            FilterChip(
                selected = level == current,
                onClick = { onSelect(level) },
                label = { Text(level.label, fontSize = 10.sp) },
                colors = FilterChipDefaults.filterChipColors(
                    selectedContainerColor = AccentMint.copy(alpha = 0.2f),
                    selectedLabelColor = AccentMint
                ),
                shape = RoundedCornerShape(10.dp)
            )
        }
    }
}

@Composable
private fun SelfCheckIntervalSlider(seconds: Int, onChange: (Int) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text("1s", fontSize = 10.sp, color = TextTertiary)
        Slider(
            value = seconds.toFloat(),
            onValueChange = { onChange(it.toInt()) },
            valueRange = 1f..30f,
            steps = 28,
            modifier = Modifier.weight(1f).padding(horizontal = 8.dp),
            colors = SliderDefaults.colors(
                thumbColor = AccentMint,
                activeTrackColor = AccentMint,
                inactiveTrackColor = AccentMint.copy(alpha = 0.15f)
            )
        )
        Text("30s", fontSize = 10.sp, color = TextTertiary)
    }
}

@Composable
private fun HideSpoofSettingsGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "隐藏 & 伪装") {
        GlassSettingsItem(
            label = "隐藏策略",
            subtitle = settings.hideStrategy.label,
            icon = Icons.Default.VisibilityOff,
            accentColor = AccentPurple
        )
        HideStrategySelector(settings.hideStrategy) { vm.updateHideStrategy(it) }
        GlassSettingsItem(
            label = "HWID 伪装",
            subtitle = if (settings.hwidSpoofEnabled) "已开启" else "已关闭",
            icon = Icons.Default.Fingerprint,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.hwidSpoofEnabled,
                    onCheckedChange = { vm.updateHwidSpoof(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "Bootloader 伪装",
            subtitle = if (settings.bootloaderSpoofEnabled) "已开启" else "已关闭",
            icon = Icons.Default.Lock,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.bootloaderSpoofEnabled,
                    onCheckedChange = { vm.updateBootloaderSpoof(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun HideStrategySelector(current: HideStrategy, onSelect: (HideStrategy) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        HideStrategy.entries.forEach { strategy ->
            FilterChip(
                selected = strategy == current,
                onClick = { onSelect(strategy) },
                label = { Text(strategy.label, fontSize = 10.sp) },
                colors = FilterChipDefaults.filterChipColors(
                    selectedContainerColor = AccentPurple.copy(alpha = 0.2f),
                    selectedLabelColor = AccentPurple
                ),
                shape = RoundedCornerShape(10.dp)
            )
        }
    }
}

@Composable
private fun GameModeSettingsGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "游戏模式") {
        GlassSettingsItem(
            label = "游戏模式",
            subtitle = if (settings.gameModeEnabled) "已开启" else "已关闭",
            icon = Icons.Default.VideogameAsset,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.gameModeEnabled,
                    onCheckedChange = { vm.updateGameMode(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "激进模式",
            subtitle = if (settings.gameModeAggressive) "隐藏所有 Root 痕迹" else "仅隐藏关键痕迹",
            icon = Icons.Default.Bolt,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.gameModeAggressive,
                    onCheckedChange = { vm.updateGameModeAggressive(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun SandboxSettingsGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "隔离环境") {
        GlassSettingsItem(
            label = "启动时自动创建",
            subtitle = if (settings.sandboxAutoCreate) "应用启动自动创建隔离环境" else "手动创建",
            icon = Icons.Default.AutoMode,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.sandboxAutoCreate,
                    onCheckedChange = { vm.updateSandboxAutoCreate(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "退出时清理",
            subtitle = if (settings.sandboxCleanupOnExit) "自动销毁隔离环境" else "保留隔离环境",
            icon = Icons.Default.CleaningServices,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.sandboxCleanupOnExit,
                    onCheckedChange = { vm.updateSandboxCleanupOnExit(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun CureSettingsGroup(settings: AppSettings, vm: SettingsViewModel) {
    val cureLabels = listOf("轻度清理", "标准修复", "深度恢复", "完全重置")
    var expanded by remember { mutableStateOf(false) }

    GlassSettingsGroup(title = "修复引擎") {
        GlassSettingsItem(
            label = "默认修复级别",
            subtitle = cureLabels.getOrElse(settings.cureDefaultLevel) { "标准修复" },
            icon = Icons.Default.Medication,
            accentColor = AccentMint,
            onClick = { expanded = !expanded }
        )
        if (expanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                horizontalArrangement = Arrangement.spacedBy(6.dp)
            ) {
                cureLabels.forEachIndexed { index, label ->
                    FilterChip(
                        selected = index == settings.cureDefaultLevel,
                        onClick = { vm.updateCureDefaultLevel(index) },
                        label = { Text(label, fontSize = 9.sp) },
                        colors = FilterChipDefaults.filterChipColors(
                            selectedContainerColor = AccentMint.copy(alpha = 0.2f),
                            selectedLabelColor = AccentMint
                        ),
                        shape = RoundedCornerShape(10.dp)
                    )
                }
            }
        }
        GlassSettingsItem(
            label = "自动修复",
            subtitle = if (settings.cureAutoFix) "检测到 Root 自动修复" else "手动触发修复",
            icon = Icons.Default.Autorenew,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.cureAutoFix,
                    onCheckedChange = { vm.updateCureAutoFix(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun AppearanceSettingsGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "外观") {
        GlassSettingsItem(
            label = "主题模式",
            subtitle = settings.themeMode.label,
            icon = Icons.Default.DarkMode,
            accentColor = AccentPurple
        )
        ThemeModeSelector(settings.themeMode) { vm.updateThemeMode(it) }
        GlassSettingsItem(
            label = "强调色",
            subtitle = settings.accentColor.label,
            icon = Icons.Default.Palette,
            accentColor = parseColor(settings.accentColor.hex)
        )
        AccentColorSelector(settings.accentColor) { vm.updateAccentColor(it) }
        GlassSettingsItem(
            label = "过渡动画",
            subtitle = if (settings.animationsEnabled) "已开启" else "已关闭",
            icon = Icons.Default.Animation,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.animationsEnabled,
                    onCheckedChange = { vm.updateAnimations(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "液态玻璃效果",
            subtitle = if (settings.liquidGlassEffect) "已开启" else "已关闭",
            icon = Icons.Default.BlurOn,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.liquidGlassEffect,
                    onCheckedChange = { vm.updateLiquidGlassEffect(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun ThemeModeSelector(current: ThemeMode, onSelect: (ThemeMode) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        ThemeMode.entries.forEach { mode ->
            FilterChip(
                selected = mode == current,
                onClick = { onSelect(mode) },
                label = { Text(mode.label, fontSize = 10.sp) },
                colors = FilterChipDefaults.filterChipColors(
                    selectedContainerColor = AccentPurple.copy(alpha = 0.2f),
                    selectedLabelColor = AccentPurple
                ),
                shape = RoundedCornerShape(10.dp)
            )
        }
    }
}

@Composable
private fun AccentColorSelector(current: AccentColor, onSelect: (AccentColor) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        AccentColor.entries.forEach { color ->
            FilterChip(
                selected = color == current,
                onClick = { onSelect(color) },
                label = { Text(color.label, fontSize = 9.sp) },
                colors = FilterChipDefaults.filterChipColors(
                    selectedContainerColor = parseColor(color.hex).copy(alpha = 0.2f),
                    selectedLabelColor = parseColor(color.hex)
                ),
                shape = RoundedCornerShape(10.dp)
            )
        }
    }
}

@Composable
private fun NetworkFirewallGroup(settings: AppSettings, vm: SettingsViewModel) {
    var proxyExpanded by remember { mutableStateOf(false) }
    var proxyText by remember(settings.updateProxy) { mutableStateOf(settings.updateProxy) }

    GlassSettingsGroup(title = "网络 & 防火墙") {
        GlassSettingsItem(
            label = "eBPF 防火墙",
            subtitle = if (settings.ebpfFirewallEnabled) "已启用" else "已禁用",
            icon = Icons.Default.Fireplace,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.ebpfFirewallEnabled,
                    onCheckedChange = { vm.updateEbpfFirewall(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "防火墙日志",
            subtitle = if (settings.firewallLogging) "已开启" else "已关闭",
            icon = Icons.Default.Assessment,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.firewallLogging,
                    onCheckedChange = { vm.updateFirewallLogging(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "更新代理",
            subtitle = if (settings.updateProxy.isBlank()) "未设置" else settings.updateProxy,
            icon = Icons.Default.VpnKey,
            accentColor = AccentBlue,
            onClick = { proxyExpanded = !proxyExpanded }
        )
        if (proxyExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = proxyText,
                    onValueChange = { proxyText = it },
                    placeholder = { Text("http://proxy:port", fontSize = 12.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentBlue,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateUpdateProxy(proxyText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentBlue),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
    }
}

@Composable
private fun NotificationsDetailGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "通知类型") {
        GlassSettingsItem(
            label = "检测完成通知",
            subtitle = if (settings.notifyScanComplete) "已开启" else "已关闭",
            icon = Icons.Default.TaskAlt,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.notifyScanComplete,
                    onCheckedChange = { vm.updateNotifyScanComplete(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "风险告警通知",
            subtitle = if (settings.notifyRiskFound) "已开启" else "已关闭",
            icon = Icons.Default.Warning,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.notifyRiskFound,
                    onCheckedChange = { vm.updateNotifyRiskFound(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "防护告警通知",
            subtitle = if (settings.notifyGuardAlert) "已开启" else "已关闭",
            icon = Icons.Default.Security,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.notifyGuardAlert,
                    onCheckedChange = { vm.updateNotifyGuardAlert(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "修复结果通知",
            subtitle = if (settings.notifyCureResult) "已开启" else "已关闭",
            icon = Icons.Default.Medication,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.notifyCureResult,
                    onCheckedChange = { vm.updateNotifyCureResult(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "更新提示通知",
            subtitle = if (settings.notifyUpdateAvailable) "已开启" else "已关闭",
            icon = Icons.Default.SystemUpdate,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.notifyUpdateAvailable,
                    onCheckedChange = { vm.updateNotifyUpdateAvailable(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun UpdatesGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "更新") {
        GlassSettingsItem(
            label = "自动检查更新",
            subtitle = if (settings.autoCheckUpdates) "已开启" else "已关闭",
            icon = Icons.Default.Update,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.autoCheckUpdates,
                    onCheckedChange = { vm.updateAutoCheckUpdates(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "更新通道",
            subtitle = settings.updateChannel.label,
            icon = Icons.Default.Rocket,
            accentColor = AccentPurple
        )
        UpdateChannelSelector(settings.updateChannel) { vm.updateUpdateChannel(it) }
        GlassSettingsItem(
            label = "仅 WiFi 更新",
            subtitle = if (settings.wifiOnlyUpdate) "仅在 WiFi 下检查更新" else "移动网络也可更新",
            icon = Icons.Default.Wifi,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.wifiOnlyUpdate,
                    onCheckedChange = { vm.updateWifiOnlyUpdate(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun UpdateChannelSelector(current: UpdateChannel, onSelect: (UpdateChannel) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        UpdateChannel.entries.forEach { channel ->
            FilterChip(
                selected = channel == current,
                onClick = { onSelect(channel) },
                label = { Text(channel.label, fontSize = 10.sp) },
                colors = FilterChipDefaults.filterChipColors(
                    selectedContainerColor = AccentPurple.copy(alpha = 0.2f),
                    selectedLabelColor = AccentPurple
                ),
                shape = RoundedCornerShape(10.dp)
            )
        }
    }
}

@Composable
private fun PrivacyGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "隐私 & 数据") {
        GlassSettingsItem(
            label = "匿名使用统计",
            subtitle = if (settings.telemetryEnabled) "帮助改进产品" else "已关闭",
            icon = Icons.Default.Analytics,
            accentColor = AccentPurple,
            trailing = {
                Switch(
                    checked = settings.telemetryEnabled,
                    onCheckedChange = { vm.updateTelemetry(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "崩溃报告",
            subtitle = if (settings.crashReportsEnabled) "自动发送崩溃报告" else "已关闭",
            icon = Icons.Default.BugReport,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.crashReportsEnabled,
                    onCheckedChange = { vm.updateCrashReports(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun PerformanceGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "性能") {
        GlassSettingsItem(
            label = "扫描并发数",
            subtitle = "${settings.scanConcurrency} 个并发任务",
            icon = Icons.Default.Speed,
            accentColor = AccentGold
        )
        ConcurrencySlider(settings.scanConcurrency) { vm.updateScanConcurrency(it) }
        GlassSettingsItem(
            label = "缓存有效期",
            subtitle = "${settings.cacheMaxAgeMinutes} 分钟",
            icon = Icons.Default.Cached,
            accentColor = AccentBlue
        )
        CacheAgeSlider(settings.cacheMaxAgeMinutes) { vm.updateCacheMaxAge(it) }
        GlassSettingsItem(
            label = "低内存模式",
            subtitle = if (settings.lowMemoryMode) "减少内存占用" else "标准模式",
            icon = Icons.Default.Memory,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.lowMemoryMode,
                    onCheckedChange = { vm.updateLowMemoryMode(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun ConcurrencySlider(count: Int, onChange: (Int) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text("1", fontSize = 10.sp, color = TextTertiary)
        Slider(
            value = count.toFloat(),
            onValueChange = { onChange(it.toInt()) },
            valueRange = 1f..8f,
            steps = 6,
            modifier = Modifier.weight(1f).padding(horizontal = 8.dp),
            colors = SliderDefaults.colors(
                thumbColor = AccentGold,
                activeTrackColor = AccentGold,
                inactiveTrackColor = AccentGold.copy(alpha = 0.15f)
            )
        )
        Text("8", fontSize = 10.sp, color = TextTertiary)
    }
}

@Composable
private fun CacheAgeSlider(minutes: Int, onChange: (Int) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text("1m", fontSize = 10.sp, color = TextTertiary)
        Slider(
            value = minutes.toFloat(),
            onValueChange = { onChange(it.toInt()) },
            valueRange = 1f..180f,
            steps = 17,
            modifier = Modifier.weight(1f).padding(horizontal = 8.dp),
            colors = SliderDefaults.colors(
                thumbColor = AccentBlue,
                activeTrackColor = AccentBlue,
                inactiveTrackColor = AccentBlue.copy(alpha = 0.15f)
            )
        )
        Text("180m", fontSize = 10.sp, color = TextTertiary)
    }
}

@Composable
private fun LoggingGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "日志") {
        GlassSettingsItem(
            label = "日志级别",
            subtitle = settings.logLevel.label,
            icon = Icons.Default.Article,
            accentColor = AccentPurple
        )
        LogLevelSelector(settings.logLevel) { vm.updateLogLevel(it) }
        GlassSettingsItem(
            label = "最大日志文件数",
            subtitle = "${settings.logMaxFiles} 个文件",
            icon = Icons.Default.Folder,
            accentColor = AccentGold
        )
        LogFilesSlider(settings.logMaxFiles) { vm.updateLogMaxFiles(it) }
        GlassSettingsItem(
            label = "自动清理日志",
            subtitle = if (settings.logAutoCleanup) "自动删除过期日志" else "手动清理",
            icon = Icons.Default.CleaningServices,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.logAutoCleanup,
                    onCheckedChange = { vm.updateLogAutoCleanup(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun LogLevelSelector(current: LogLevel, onSelect: (LogLevel) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        LogLevel.entries.forEach { level ->
            FilterChip(
                selected = level == current,
                onClick = { onSelect(level) },
                label = { Text(level.label, fontSize = 10.sp) },
                colors = FilterChipDefaults.filterChipColors(
                    selectedContainerColor = AccentPurple.copy(alpha = 0.2f),
                    selectedLabelColor = AccentPurple
                ),
                shape = RoundedCornerShape(10.dp)
            )
        }
    }
}

@Composable
private fun LogFilesSlider(count: Int, onChange: (Int) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text("1", fontSize = 10.sp, color = TextTertiary)
        Slider(
            value = count.toFloat(),
            onValueChange = { onChange(it.toInt()) },
            valueRange = 1f..50f,
            steps = 48,
            modifier = Modifier.weight(1f).padding(horizontal = 8.dp),
            colors = SliderDefaults.colors(
                thumbColor = AccentGold,
                activeTrackColor = AccentGold,
                inactiveTrackColor = AccentGold.copy(alpha = 0.15f)
            )
        )
        Text("50", fontSize = 10.sp, color = TextTertiary)
    }
}

@Composable
private fun SchedulingGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "定时任务") {
        GlassSettingsItem(
            label = "定时扫描",
            subtitle = settings.scheduleTime.label,
            icon = Icons.Default.Schedule,
            accentColor = AccentPurple
        )
        ScheduleTimeSelector(settings.scheduleTime) { vm.updateScheduleTime(it) }
    }
}

@Composable
private fun ScheduleTimeSelector(current: ScanScheduleTime, onSelect: (ScanScheduleTime) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
        horizontalArrangement = Arrangement.spacedBy(6.dp)
    ) {
        ScanScheduleTime.entries.forEach { time ->
            FilterChip(
                selected = time == current,
                onClick = { onSelect(time) },
                label = { Text(time.label, fontSize = 9.sp) },
                colors = FilterChipDefaults.filterChipColors(
                    selectedContainerColor = AccentPurple.copy(alpha = 0.2f),
                    selectedLabelColor = AccentPurple
                ),
                shape = RoundedCornerShape(10.dp)
            )
        }
    }
}

@Composable
private fun AdvancedGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "高级") {
        GlassSettingsItem(
            label = "语言",
            subtitle = settings.language.label,
            icon = Icons.Default.Language,
            accentColor = AccentPurple
        )
        LanguageSelector(settings.language) { vm.updateLanguage(it) }
        GlassSettingsItem(
            label = "实验性功能",
            subtitle = if (settings.experimentalFeatures) "已开启" else "已关闭",
            icon = Icons.Default.Science,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.experimentalFeatures,
                    onCheckedChange = { vm.updateExperimentalFeatures(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun LanguageSelector(current: Language, onSelect: (Language) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        Language.entries.forEach { lang ->
            FilterChip(
                selected = lang == current,
                onClick = { onSelect(lang) },
                label = { Text(lang.label, fontSize = 10.sp) },
                colors = FilterChipDefaults.filterChipColors(
                    selectedContainerColor = AccentPurple.copy(alpha = 0.2f),
                    selectedLabelColor = AccentPurple
                ),
                shape = RoundedCornerShape(10.dp)
            )
        }
    }
}

@Composable
private fun PowerSettingsGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "电源") {
        GlassSettingsItem(
            label = "性能配置",
            subtitle = settings.powerProfile.label,
            icon = Icons.Default.BatteryFull,
            accentColor = AccentMint
        )
        PowerProfileSelector(settings.powerProfile) { vm.updatePowerProfile(it) }
        GlassSettingsItem(
            label = "省电模式",
            subtitle = if (settings.batterySaverEnabled) "限制后台活动以节省电量" else "已关闭",
            icon = Icons.Default.EnergySavingsLeaf,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.batterySaverEnabled,
                    onCheckedChange = { vm.updateBatterySaver(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun PowerProfileSelector(current: PowerProfile, onSelect: (PowerProfile) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        PowerProfile.entries.forEach { profile ->
            FilterChip(
                selected = profile == current,
                onClick = { onSelect(profile) },
                label = { Text(profile.label, fontSize = 10.sp) },
                colors = FilterChipDefaults.filterChipColors(
                    selectedContainerColor = AccentMint.copy(alpha = 0.2f),
                    selectedLabelColor = AccentMint
                ),
                shape = RoundedCornerShape(10.dp)
            )
        }
    }
}

@Composable
private fun IpcSettingsGroup(settings: AppSettings, vm: SettingsViewModel) {
    var socketExpanded by remember { mutableStateOf(false) }
    var socketText by remember(settings.ipcSocketPath) { mutableStateOf(settings.ipcSocketPath) }

    GlassSettingsGroup(title = "IPC 通信") {
        GlassSettingsItem(
            label = "通信超时",
            subtitle = "${settings.ipcTimeoutSeconds} 秒",
            icon = Icons.Default.Timer,
            accentColor = AccentBlue
        )
        IpcTimeoutSlider(settings.ipcTimeoutSeconds) { vm.updateIpcTimeout(it) }
        GlassSettingsItem(
            label = "重试次数",
            subtitle = "${settings.ipcRetryCount} 次",
            icon = Icons.Default.Replay,
            accentColor = AccentGold
        )
        IpcRetrySlider(settings.ipcRetryCount) { vm.updateIpcRetryCount(it) }
        GlassSettingsItem(
            label = "Socket 路径",
            subtitle = if (settings.ipcSocketPath.isBlank()) "默认路径" else settings.ipcSocketPath,
            icon = Icons.Default.Cable,
            accentColor = AccentPurple,
            onClick = { socketExpanded = !socketExpanded }
        )
        if (socketExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = socketText,
                    onValueChange = { socketText = it },
                    placeholder = { Text("apex_root_sandbox", fontSize = 12.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentPurple,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateIpcSocketPath(socketText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentPurple),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
    }
}

@Composable
private fun IpcTimeoutSlider(seconds: Int, onChange: (Int) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text("1s", fontSize = 10.sp, color = TextTertiary)
        Slider(
            value = seconds.toFloat(),
            onValueChange = { onChange(it.toInt()) },
            valueRange = 1f..60f,
            steps = 58,
            modifier = Modifier.weight(1f).padding(horizontal = 8.dp),
            colors = SliderDefaults.colors(
                thumbColor = AccentBlue,
                activeTrackColor = AccentBlue,
                inactiveTrackColor = AccentBlue.copy(alpha = 0.15f)
            )
        )
        Text("60s", fontSize = 10.sp, color = TextTertiary)
    }
}

@Composable
private fun IpcRetrySlider(count: Int, onChange: (Int) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text("0", fontSize = 10.sp, color = TextTertiary)
        Slider(
            value = count.toFloat(),
            onValueChange = { onChange(it.toInt()) },
            valueRange = 0f..20f,
            steps = 19,
            modifier = Modifier.weight(1f).padding(horizontal = 8.dp),
            colors = SliderDefaults.colors(
                thumbColor = AccentGold,
                activeTrackColor = AccentGold,
                inactiveTrackColor = AccentGold.copy(alpha = 0.15f)
            )
        )
        Text("20", fontSize = 10.sp, color = TextTertiary)
    }
}

@Composable
private fun WatchdogGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "看门狗") {
        GlassSettingsItem(
            label = "健康检查",
            subtitle = if (settings.watchdogEnabled) "定期检查进程状态" else "已关闭",
            icon = Icons.Default.Monitor,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.watchdogEnabled,
                    onCheckedChange = { vm.updateWatchdogEnabled(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        if (settings.watchdogEnabled) {
            GlassSettingsItem(
                label = "检查间隔",
                subtitle = "${settings.watchdogIntervalSeconds} 秒",
                icon = Icons.Default.Timer,
                accentColor = AccentBlue
            )
            WatchdogIntervalSlider(settings.watchdogIntervalSeconds) { vm.updateWatchdogInterval(it) }
            GlassSettingsItem(
                label = "自动重启",
                subtitle = if (settings.watchdogAutoRestart) "检测到异常自动重启进程" else "仅告警",
                icon = Icons.Default.RestartAlt,
                accentColor = ErrorRed,
                trailing = {
                    Switch(
                        checked = settings.watchdogAutoRestart,
                        onCheckedChange = { vm.updateWatchdogAutoRestart(it) },
                        colors = SwitchDefaults.colors(
                            checkedThumbColor = AccentPurple,
                            checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                        )
                    )
                }
            )
        }
    }
}

@Composable
private fun WatchdogIntervalSlider(seconds: Int, onChange: (Int) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text("5s", fontSize = 10.sp, color = TextTertiary)
        Slider(
            value = seconds.toFloat(),
            onValueChange = { onChange(it.toInt()) },
            valueRange = 5f..300f,
            steps = 58,
            modifier = Modifier.weight(1f).padding(horizontal = 8.dp),
            colors = SliderDefaults.colors(
                thumbColor = AccentBlue,
                activeTrackColor = AccentBlue,
                inactiveTrackColor = AccentBlue.copy(alpha = 0.15f)
            )
        )
        Text("300s", fontSize = 10.sp, color = TextTertiary)
    }
}

@Composable
private fun AntiDetectionGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "反检测") {
        GlassSettingsItem(
            label = "混淆级别",
            subtitle = settings.obfuscationLevel.label,
            icon = Icons.Default.Blind,
            accentColor = AccentPurple
        )
        ObfuscationLevelSelector(settings.obfuscationLevel) { vm.updateObfuscationLevel(it) }
        GlassSettingsItem(
            label = "CPU 随机绑定",
            subtitle = if (settings.cpuRandomAffinity) "每次检测随机绑定 CPU 核心" else "固定核心",
            icon = Icons.Default.Memory,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.cpuRandomAffinity,
                    onCheckedChange = { vm.updateCpuRandomAffinity(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun ObfuscationLevelSelector(current: ObfuscationLevel, onSelect: (ObfuscationLevel) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        ObfuscationLevel.entries.forEach { level ->
            FilterChip(
                selected = level == current,
                onClick = { onSelect(level) },
                label = { Text(level.label, fontSize = 10.sp) },
                colors = FilterChipDefaults.filterChipColors(
                    selectedContainerColor = AccentPurple.copy(alpha = 0.2f),
                    selectedLabelColor = AccentPurple
                ),
                shape = RoundedCornerShape(10.dp)
            )
        }
    }
}

@Composable
private fun WhitelistGroup(settings: AppSettings, vm: SettingsViewModel) {
    var whitelistExpanded by remember { mutableStateOf(false) }
    var exclusionExpanded by remember { mutableStateOf(false) }
    var keywordExpanded by remember { mutableStateOf(false) }
    var hidePathExpanded by remember { mutableStateOf(false) }
    var whitelistText by remember(settings.whitelistApps) { mutableStateOf(settings.whitelistApps) }
    var exclusionText by remember(settings.scanExclusions) { mutableStateOf(settings.scanExclusions) }
    var keywordText by remember(settings.customDetectionKeywords) { mutableStateOf(settings.customDetectionKeywords) }
    var hidePathText by remember(settings.customHidePaths) { mutableStateOf(settings.customHidePaths) }

    GlassSettingsGroup(title = "白名单 & 排除") {
        GlassSettingsItem(
            label = "应用白名单",
            subtitle = if (settings.whitelistApps.isBlank()) "未设置" else "${settings.whitelistApps.split(",").size} 个应用",
            icon = Icons.Default.Apps,
            accentColor = AccentMint,
            onClick = { whitelistExpanded = !whitelistExpanded }
        )
        if (whitelistExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = whitelistText,
                    onValueChange = { whitelistText = it },
                    placeholder = { Text("com.example.app,com.other.app", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentMint,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateWhitelistApps(whitelistText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentMint),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
        GlassSettingsItem(
            label = "扫描排除路径",
            subtitle = if (settings.scanExclusions.isBlank()) "未设置" else "${settings.scanExclusions.split(",").size} 个路径",
            icon = Icons.Default.FolderOff,
            accentColor = AccentBlue,
            onClick = { exclusionExpanded = !exclusionExpanded }
        )
        if (exclusionExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = exclusionText,
                    onValueChange = { exclusionText = it },
                    placeholder = { Text("/data/local/tmp,/sdcard/Download", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentBlue,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateScanExclusions(exclusionText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentBlue),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
        GlassSettingsItem(
            label = "自定义检测关键词",
            subtitle = if (settings.customDetectionKeywords.isBlank()) "未设置" else "${settings.customDetectionKeywords.split(",").size} 个关键词",
            icon = Icons.Default.ManageSearch,
            accentColor = AccentGold,
            onClick = { keywordExpanded = !keywordExpanded }
        )
        if (keywordExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = keywordText,
                    onValueChange = { keywordText = it },
                    placeholder = { Text("magisk,zygisk,ksu", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentGold,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateCustomDetectionKeywords(keywordText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentGold),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
        GlassSettingsItem(
            label = "自定义隐藏路径",
            subtitle = if (settings.customHidePaths.isBlank()) "未设置" else "${settings.customHidePaths.split(",").size} 个路径",
            icon = Icons.Default.HideSource,
            accentColor = ErrorRed,
            onClick = { hidePathExpanded = !hidePathExpanded }
        )
        if (hidePathExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = hidePathText,
                    onValueChange = { hidePathText = it },
                    placeholder = { Text("/data/adb/magisk,/system/bin/su", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = ErrorRed,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateCustomHidePaths(hidePathText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = ErrorRed),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
    }
}

@Composable
private fun HardeningGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "安全加固") {
        GlassSettingsItem(
            label = "Boot 校验",
            subtitle = if (settings.verifyBootEnabled) "验证 Boot 分区完整性" else "已关闭",
            icon = Icons.Default.Verified,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.verifyBootEnabled,
                    onCheckedChange = { vm.updateVerifyBoot(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "KeyStore 校验",
            subtitle = if (settings.keystoreCheckEnabled) "验证 KeyStore 完整性" else "已关闭",
            icon = Icons.Default.Key,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.keystoreCheckEnabled,
                    onCheckedChange = { vm.updateKeystoreCheck(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "SELinux 强制检查",
            subtitle = if (settings.selinuxEnforceCheck) "检测 SELinux 状态是否被修改" else "已关闭",
            icon = Icons.Default.Security,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.selinuxEnforceCheck,
                    onCheckedChange = { vm.updateSelinuxEnforceCheck(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun StealthGroup(settings: AppSettings, vm: SettingsViewModel) {
    var fakeNameExpanded by remember { mutableStateOf(false) }
    var fakeNameText by remember(settings.stealthFakeAppName) { mutableStateOf(settings.stealthFakeAppName) }

    GlassSettingsGroup(title = "隐身") {
        GlassSettingsItem(
            label = "隐藏桌面图标",
            subtitle = if (settings.stealthHideIcon) "启动器中将不再显示应用图标" else "已关闭",
            icon = Icons.Default.VisibilityOff,
            accentColor = AccentPurple,
            trailing = {
                Switch(
                    checked = settings.stealthHideIcon,
                    onCheckedChange = { vm.updateStealthHideIcon(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "隐藏最近任务",
            subtitle = if (settings.stealthHideRecent) "在多任务界面隐藏应用" else "已关闭",
            icon = Icons.Default.HideImage,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.stealthHideRecent,
                    onCheckedChange = { vm.updateStealthHideRecent(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "隐藏通知栏",
            subtitle = if (settings.stealthHideNotification) "通知栏中不显示任何来自本应用的通知" else "已关闭",
            icon = Icons.Default.NotificationsOff,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.stealthHideNotification,
                    onCheckedChange = { vm.updateStealthHideNotification(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "伪装应用名称",
            subtitle = if (settings.stealthFakeAppName.isNotBlank()) settings.stealthFakeAppName else "未设置",
            icon = Icons.Default.Edit,
            accentColor = AccentMint,
            onClick = { fakeNameExpanded = !fakeNameExpanded }
        )
        if (fakeNameExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = fakeNameText,
                    onValueChange = { fakeNameText = it },
                    placeholder = { Text("系统工具", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentMint,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateStealthFakeAppName(fakeNameText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentMint),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
    }
}

@Composable
private fun EmergencyGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "应急响应") {
        GlassSettingsItem(
            label = "锁定设备",
            subtitle = if (settings.emergencyLockDevice) "检测到风险时立即锁定设备" else "已关闭",
            icon = Icons.Default.Lock,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.emergencyLockDevice,
                    onCheckedChange = { vm.updateEmergencyLockDevice(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = ErrorRed,
                        checkedTrackColor = ErrorRed.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "擦除数据",
            subtitle = if (settings.emergencyWipeData) "检测到严重风险时远程擦除数据" else "已关闭",
            icon = Icons.Default.DeleteForever,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.emergencyWipeData,
                    onCheckedChange = { vm.updateEmergencyWipeData(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = ErrorRed,
                        checkedTrackColor = ErrorRed.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "关机保护",
            subtitle = if (settings.emergencyShutdown) "检测到入侵时自动关机" else "已关闭",
            icon = Icons.Default.PowerOff,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.emergencyShutdown,
                    onCheckedChange = { vm.updateEmergencyShutdown(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = ErrorRed,
                        checkedTrackColor = ErrorRed.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "通知紧急联系人",
            subtitle = if (settings.emergencyNotifyContacts) "检测到风险时发送警报至联系人" else "已关闭",
            icon = Icons.Default.ContactEmergency,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.emergencyNotifyContacts,
                    onCheckedChange = { vm.updateEmergencyNotifyContacts(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun SideChannelGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "侧信道防护") {
        GlassSettingsItem(
            label = "PMU 伪造",
            subtitle = if (settings.pmuSpoofEnabled) "伪造性能监控单元读数" else "已关闭",
            icon = Icons.Default.Speed,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.pmuSpoofEnabled,
                    onCheckedChange = { vm.updatePmuSpoof(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "时间侧信道伪造",
            subtitle = if (settings.timingSpoofEnabled) "伪造指令执行时间分布" else "已关闭",
            icon = Icons.Default.Timer,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.timingSpoofEnabled,
                    onCheckedChange = { vm.updateTimingSpoof(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "内存侧信道伪造",
            subtitle = if (settings.memorySpoofEnabled) "伪造内存访问模式" else "已关闭",
            icon = Icons.Default.Memory,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.memorySpoofEnabled,
                    onCheckedChange = { vm.updateMemorySpoof(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun SoundHapticGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "声音 & 触感") {
        GlassSettingsItem(
            label = "扫描完成音效",
            subtitle = if (settings.soundScanComplete) "扫描完成时播放提示音" else "已关闭",
            icon = Icons.Default.TaskAlt,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.soundScanComplete,
                    onCheckedChange = { vm.updateSoundScanComplete(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "风险警报音效",
            subtitle = if (settings.soundRiskAlert) "发现风险时播放警报音" else "已关闭",
            icon = Icons.Default.Warning,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.soundRiskAlert,
                    onCheckedChange = { vm.updateSoundRiskAlert(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = ErrorRed,
                        checkedTrackColor = ErrorRed.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "防护触发音效",
            subtitle = if (settings.soundGuardAlert) "防护机制触发时播放提示音" else "已关闭",
            icon = Icons.Default.Shield,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.soundGuardAlert,
                    onCheckedChange = { vm.updateSoundGuardAlert(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "触感反馈",
            subtitle = if (settings.hapticFeedback) "交互时提供震动反馈" else "已关闭",
            icon = Icons.Default.Vibration,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.hapticFeedback,
                    onCheckedChange = { vm.updateHapticFeedback(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "游戏时自动静音",
            subtitle = if (settings.autoSilenceOnGame) "游戏运行时自动静音所有提示音" else "已关闭",
            icon = Icons.Default.VolumeMute,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.autoSilenceOnGame,
                    onCheckedChange = { vm.updateAutoSilenceOnGame(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun AppLockGroup(settings: AppSettings, vm: SettingsViewModel) {
    var pinExpanded by remember { mutableStateOf(false) }
    var pinText by remember(settings.appLockPin) { mutableStateOf(settings.appLockPin) }

    GlassSettingsGroup(title = "应用锁") {
        GlassSettingsItem(
            label = "启用应用锁",
            subtitle = if (settings.appLockEnabled) "进入应用时需要验证" else "已关闭",
            icon = Icons.Default.Lock,
            accentColor = AccentPurple,
            trailing = {
                Switch(
                    checked = settings.appLockEnabled,
                    onCheckedChange = { vm.updateAppLockEnabled(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "PIN 码",
            subtitle = if (settings.appLockPin.isNotBlank()) "已设置" else "未设置",
            icon = Icons.Default.Pin,
            accentColor = AccentGold,
            onClick = { pinExpanded = !pinExpanded }
        )
        if (pinExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = pinText,
                    onValueChange = { pinText = it },
                    placeholder = { Text("输入 4-6 位 PIN 码", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentGold,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateAppLockPin(pinText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentGold),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
        GlassSettingsItem(
            label = "生物识别解锁",
            subtitle = if (settings.appLockBiometric) "支持指纹 / 面部识别解锁" else "已关闭",
            icon = Icons.Default.Fingerprint,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.appLockBiometric,
                    onCheckedChange = { vm.updateAppLockBiometric(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun ExportGroup(settings: AppSettings, vm: SettingsViewModel) {
    var formatExpanded by remember { mutableStateOf(false) }
    var formatText by remember(settings.exportFormat) { mutableStateOf(settings.exportFormat) }

    GlassSettingsGroup(title = "导出") {
        GlassSettingsItem(
            label = "自动导出报告",
            subtitle = if (settings.autoExportEnabled) "每次检测后自动导出报告" else "已关闭",
            icon = Icons.Default.FileUpload,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.autoExportEnabled,
                    onCheckedChange = { vm.updateAutoExport(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "导出格式",
            subtitle = settings.exportFormat.uppercase(),
            icon = Icons.Default.Description,
            accentColor = AccentBlue,
            onClick = { formatExpanded = !formatExpanded }
        )
        if (formatExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = formatText,
                    onValueChange = { formatText = it },
                    placeholder = { Text("json / csv / html", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentBlue,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateExportFormat(formatText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentBlue),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
    }
}

@Composable
private fun CryptoGroup(settings: AppSettings, vm: SettingsViewModel) {
    var keyExpanded by remember { mutableStateOf(false) }
    var keyText by remember(settings.customEncryptionKey) { mutableStateOf(settings.customEncryptionKey) }

    GlassSettingsGroup(title = "加密") {
        GlassSettingsItem(
            label = "自定义加密密钥",
            subtitle = if (settings.customEncryptionKey.isNotBlank()) "已设置" else "未设置",
            icon = Icons.Default.Key,
            accentColor = AccentGold,
            onClick = { keyExpanded = !keyExpanded }
        )
        if (keyExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = keyText,
                    onValueChange = { keyText = it },
                    placeholder = { Text("输入加密密钥", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentGold,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateCustomEncryptionKey(keyText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentGold),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
        GlassSettingsItem(
            label = "密钥轮换周期",
            subtitle = "${settings.keyRotationHours} 小时",
            icon = Icons.Default.Update,
            accentColor = AccentBlue
        )
        Row(
            modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text("1h", fontSize = 10.sp, color = TextTertiary)
            Slider(
                value = settings.keyRotationHours.toFloat(),
                onValueChange = { vm.updateKeyRotationHours(it.toInt()) },
                valueRange = 1f..8760f,
                steps = 23,
                modifier = Modifier.weight(1f).padding(horizontal = 8.dp),
                colors = SliderDefaults.colors(
                    thumbColor = AccentBlue,
                    activeTrackColor = AccentBlue,
                    inactiveTrackColor = AccentBlue.copy(alpha = 0.15f)
                )
            )
            Text("8760h", fontSize = 10.sp, color = TextTertiary)
        }
    }
}

@Composable
private fun NetworkMonitorGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "网络监控") {
        GlassSettingsItem(
            label = "DNS 监控",
            subtitle = if (settings.networkDnsMonitor) "监控所有 DNS 查询请求" else "已关闭",
            icon = Icons.Default.Dns,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.networkDnsMonitor,
                    onCheckedChange = { vm.updateNetworkDnsMonitor(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "流量监控",
            subtitle = if (settings.networkTrafficMonitor) "监控网络流量并记录" else "已关闭",
            icon = Icons.Default.Traffic,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.networkTrafficMonitor,
                    onCheckedChange = { vm.updateNetworkTrafficMonitor(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun KernelGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "内核") {
        GlassSettingsItem(
            label = "内核模块检查",
            subtitle = if (settings.kernelModuleCheckEnabled) "检测已加载的内核模块" else "已关闭",
            icon = Icons.Default.Memory,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.kernelModuleCheckEnabled,
                    onCheckedChange = { vm.updateKernelModuleCheck(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

private fun parseColor(hex: String): Color {
    try {
        val colorLong = hex.removePrefix("#").toLong(16)
        return Color(
            red = ((colorLong shr 16) and 0xFF).toInt(),
            green = ((colorLong shr 8) and 0xFF).toInt(),
            blue = (colorLong and 0xFF).toInt()
        )
    } catch (_: Exception) {
        return AccentPurple
    }
}

@Composable
private fun QuickActionsGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "快捷操作") {
        GlassSettingsItem(
            label = "快速扫描开关",
            subtitle = if (settings.quickScanToggle) "通知栏显示快速扫描快捷开关" else "已关闭",
            icon = Icons.Default.ToggleOn,
            accentColor = AccentPurple,
            trailing = {
                Switch(
                    checked = settings.quickScanToggle,
                    onCheckedChange = { vm.updateQuickScanToggle(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "悬浮球",
            subtitle = if (settings.floatingWidget) "显示悬浮球快速操作" else "已关闭",
            icon = Icons.Default.Adjust,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.floatingWidget,
                    onCheckedChange = { vm.updateFloatingWidget(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "手势快捷操作",
            subtitle = if (settings.gestureShortcut) "支持手势触发快捷操作" else "已关闭",
            icon = Icons.Default.Gesture,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.gestureShortcut,
                    onCheckedChange = { vm.updateGestureShortcut(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "桌面小部件",
            subtitle = if (settings.homeScreenWidget) "在主屏幕添加检测小部件" else "已关闭",
            icon = Icons.Default.Widgets,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.homeScreenWidget,
                    onCheckedChange = { vm.updateHomeScreenWidget(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun BackupRestoreGroup(settings: AppSettings, vm: SettingsViewModel) {
    var backupPathExpanded by remember { mutableStateOf(false) }
    var backupPathText by remember(settings.backupPath) { mutableStateOf(settings.backupPath) }

    GlassSettingsGroup(title = "备份 & 恢复") {
        GlassSettingsItem(
            label = "自动备份",
            subtitle = if (settings.autoBackupEnabled) "定期自动备份设置和报告" else "已关闭",
            icon = Icons.Default.Backup,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.autoBackupEnabled,
                    onCheckedChange = { vm.updateAutoBackup(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "备份间隔",
            subtitle = "${settings.backupIntervalDays} 天",
            icon = Icons.Default.Schedule,
            accentColor = AccentGold
        )
        Row(
            modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text("1", fontSize = 10.sp, color = TextTertiary)
            Slider(
                value = settings.backupIntervalDays.toFloat(),
                onValueChange = { vm.updateBackupIntervalDays(it.toInt()) },
                valueRange = 1f..90f,
                steps = 17,
                modifier = Modifier.weight(1f).padding(horizontal = 8.dp),
                colors = SliderDefaults.colors(
                    thumbColor = AccentGold,
                    activeTrackColor = AccentGold,
                    inactiveTrackColor = AccentGold.copy(alpha = 0.15f)
                )
            )
            Text("90", fontSize = 10.sp, color = TextTertiary)
        }
        GlassSettingsItem(
            label = "备份存储路径",
            subtitle = if (settings.backupPath.isNotBlank()) settings.backupPath else "默认位置",
            icon = Icons.Default.Folder,
            accentColor = AccentMint,
            onClick = { backupPathExpanded = !backupPathExpanded }
        )
        if (backupPathExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = backupPathText,
                    onValueChange = { backupPathText = it },
                    placeholder = { Text("/sdcard/APEX-Root/backup", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentMint,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateBackupPath(backupPathText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentMint),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
        GlassSettingsItem(
            label = "云同步",
            subtitle = if (settings.cloudSyncEnabled) "同步设置到云端" else "已关闭",
            icon = Icons.Default.CloudSync,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.cloudSyncEnabled,
                    onCheckedChange = { vm.updateCloudSync(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun ConnectivityGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "连接管理") {
        GlassSettingsItem(
            label = "USB 连接检测",
            subtitle = if (settings.usbDetectionAlert) "USB 连接时发出警报" else "已关闭",
            icon = Icons.Default.Usb,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.usbDetectionAlert,
                    onCheckedChange = { vm.updateUsbDetectionAlert(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "WiFi 安全扫描",
            subtitle = if (settings.wifiSecurityScan) "扫描已连接 WiFi 的安全性" else "已关闭",
            icon = Icons.Default.Wifi,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.wifiSecurityScan,
                    onCheckedChange = { vm.updateWifiSecurityScan(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "蓝牙 LE 扫描",
            subtitle = if (settings.bleScanEnabled) "扫描附近蓝牙低功耗设备" else "已关闭",
            icon = Icons.Default.Bluetooth,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.bleScanEnabled,
                    onCheckedChange = { vm.updateBleScan(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "不可信网络警报",
            subtitle = if (settings.untrustedNetworkAlert) "连接到不可信网络时发出警报" else "已关闭",
            icon = Icons.Default.NetworkCheck,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.untrustedNetworkAlert,
                    onCheckedChange = { vm.updateUntrustedNetworkAlert(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun AutomationGroup(settings: AppSettings, vm: SettingsViewModel) {
    var webhookExpanded by remember { mutableStateOf(false) }
    var webhookText by remember(settings.webhookUrl) { mutableStateOf(settings.webhookUrl) }

    GlassSettingsGroup(title = "自动化") {
        GlassSettingsItem(
            label = "Tasker 插件",
            subtitle = if (settings.taskerPluginEnabled) "允许 Tasker 控制检测和防护" else "已关闭",
            icon = Icons.Default.Autorenew,
            accentColor = AccentPurple,
            trailing = {
                Switch(
                    checked = settings.taskerPluginEnabled,
                    onCheckedChange = { vm.updateTaskerPlugin(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "HTTP Webhook",
            subtitle = if (settings.webhookUrl.isNotBlank()) "已配置" else "未设置",
            icon = Icons.Default.Webhook,
            accentColor = AccentBlue,
            onClick = { webhookExpanded = !webhookExpanded }
        )
        if (webhookExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = webhookText,
                    onValueChange = { webhookText = it },
                    placeholder = { Text("https://hooks.example.com/alert", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentBlue,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateWebhookUrl(webhookText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentBlue),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
        GlassSettingsItem(
            label = "Intent 触发",
            subtitle = if (settings.intentTriggerEnabled) "通过 Intent 广播触发操作" else "已关闭",
            icon = Icons.Default.RadioButtonChecked,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.intentTriggerEnabled,
                    onCheckedChange = { vm.updateIntentTrigger(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "SMS 命令",
            subtitle = if (settings.smsCommandsEnabled) "通过短信执行远程命令" else "已关闭",
            icon = Icons.Default.Sms,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.smsCommandsEnabled,
                    onCheckedChange = { vm.updateSmsCommands(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = ErrorRed,
                        checkedTrackColor = ErrorRed.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun RemoteControlGroup(settings: AppSettings, vm: SettingsViewModel) {
    var pairingExpanded by remember { mutableStateOf(false) }
    var pairingText by remember(settings.pairingCode) { mutableStateOf(settings.pairingCode) }

    GlassSettingsGroup(title = "远程管理") {
        GlassSettingsItem(
            label = "远程配对",
            subtitle = if (settings.remotePairingEnabled) "允许远程设备配对管理" else "已关闭",
            icon = Icons.Default.CastConnected,
            accentColor = AccentPurple,
            trailing = {
                Switch(
                    checked = settings.remotePairingEnabled,
                    onCheckedChange = { vm.updateRemotePairing(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "FCM 远程命令",
            subtitle = if (settings.fcmRemoteCommands) "通过云端推送执行远程命令" else "已关闭",
            icon = Icons.Default.CloudQueue,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.fcmRemoteCommands,
                    onCheckedChange = { vm.updateFcmRemoteCommands(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "配对码",
            subtitle = if (settings.pairingCode.isNotBlank()) "已设置" else "未设置",
            icon = Icons.Default.VpnKey,
            accentColor = AccentGold,
            onClick = { pairingExpanded = !pairingExpanded }
        )
        if (pairingExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = pairingText,
                    onValueChange = { pairingText = it },
                    placeholder = { Text("输入 6 位配对码", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentGold,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updatePairingCode(pairingText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentGold),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
        GlassSettingsItem(
            label = "自动信任已配对设备",
            subtitle = if (settings.autoPairTrusted) "已配对设备自动连接" else "需要手动确认",
            icon = Icons.Default.VerifiedUser,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.autoPairTrusted,
                    onCheckedChange = { vm.updateAutoPairTrusted(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun UserInterfaceGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "用户界面") {
        GlassSettingsItem(
            label = "字体缩放",
            subtitle = "${settings.fontScale}%",
            icon = Icons.Default.TextFields,
            accentColor = AccentBlue
        )
        Row(
            modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text("50%", fontSize = 10.sp, color = TextTertiary)
            Slider(
                value = settings.fontScale.toFloat(),
                onValueChange = { vm.updateFontScale(it.toInt()) },
                valueRange = 50f..200f,
                steps = 30,
                modifier = Modifier.weight(1f).padding(horizontal = 8.dp),
                colors = SliderDefaults.colors(
                    thumbColor = AccentBlue,
                    activeTrackColor = AccentBlue,
                    inactiveTrackColor = AccentBlue.copy(alpha = 0.15f)
                )
            )
            Text("200%", fontSize = 10.sp, color = TextTertiary)
        }
        GlassSettingsItem(
            label = "紧凑模式",
            subtitle = if (settings.compactModeEnabled) "减少间距显示更多内容" else "标准间距",
            icon = Icons.Default.ViewCompact,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.compactModeEnabled,
                    onCheckedChange = { vm.updateCompactMode(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "状态栏指示器",
            subtitle = if (settings.statusBarIndicator) "状态栏显示防护状态图标" else "已关闭",
            icon = Icons.Default.RadioButtonChecked,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.statusBarIndicator,
                    onCheckedChange = { vm.updateStatusBarIndicator(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "悬浮窗信息",
            subtitle = if (settings.floatingWidgetEnabled) "在其他应用上层显示悬浮窗" else "已关闭",
            icon = Icons.Default.PictureInPictureAlt,
            accentColor = AccentPurple,
            trailing = {
                Switch(
                    checked = settings.floatingWidgetEnabled,
                    onCheckedChange = { vm.updateFloatingWidgetEnabled(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun StorageGroup(settings: AppSettings, vm: SettingsViewModel) {
    var storagePathExpanded by remember { mutableStateOf(false) }
    var storagePathText by remember(settings.reportStoragePath) { mutableStateOf(settings.reportStoragePath) }

    GlassSettingsGroup(title = "存储管理") {
        GlassSettingsItem(
            label = "报告存储路径",
            subtitle = if (settings.reportStoragePath.isNotBlank()) settings.reportStoragePath else "默认位置",
            icon = Icons.Default.FolderOpen,
            accentColor = AccentBlue,
            onClick = { storagePathExpanded = !storagePathExpanded }
        )
        if (storagePathExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = storagePathText,
                    onValueChange = { storagePathText = it },
                    placeholder = { Text("/sdcard/APEX-Root/reports", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentBlue,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateReportStoragePath(storagePathText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentBlue),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
        GlassSettingsItem(
            label = "退出时清理缓存",
            subtitle = if (settings.cacheCleanupOnExit) "应用退出时自动清理缓存" else "保留缓存",
            icon = Icons.Default.CleaningServices,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.cacheCleanupOnExit,
                    onCheckedChange = { vm.updateCacheCleanupOnExit(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "最大缓存大小",
            subtitle = "${settings.maxCacheSizeMb} MB",
            icon = Icons.Default.Storage,
            accentColor = AccentGold
        )
        Row(
            modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text("10", fontSize = 10.sp, color = TextTertiary)
            Slider(
                value = settings.maxCacheSizeMb.toFloat(),
                onValueChange = { vm.updateMaxCacheSizeMb(it.toInt()) },
                valueRange = 10f..500f,
                steps = 48,
                modifier = Modifier.weight(1f).padding(horizontal = 8.dp),
                colors = SliderDefaults.colors(
                    thumbColor = AccentGold,
                    activeTrackColor = AccentGold,
                    inactiveTrackColor = AccentGold.copy(alpha = 0.15f)
                )
            )
            Text("500", fontSize = 10.sp, color = TextTertiary)
        }
        GlassSettingsItem(
            label = "备份保留天数",
            subtitle = "${settings.backupRetentionDays} 天",
            icon = Icons.Default.CalendarMonth,
            accentColor = ErrorRed
        )
        Row(
            modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text("1", fontSize = 10.sp, color = TextTertiary)
            Slider(
                value = settings.backupRetentionDays.toFloat(),
                onValueChange = { vm.updateBackupRetentionDays(it.toInt()) },
                valueRange = 1f..365f,
                steps = 51,
                modifier = Modifier.weight(1f).padding(horizontal = 8.dp),
                colors = SliderDefaults.colors(
                    thumbColor = ErrorRed,
                    activeTrackColor = ErrorRed,
                    inactiveTrackColor = ErrorRed.copy(alpha = 0.15f)
                )
            )
            Text("365", fontSize = 10.sp, color = TextTertiary)
        }
    }
}

@Composable
private fun DeveloperGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "调试 & 开发者") {
        GlassSettingsItem(
            label = "ADB 监控",
            subtitle = if (settings.adbMonitorEnabled) "监控 ADB 连接状态变化" else "已关闭",
            icon = Icons.Default.DeveloperMode,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.adbMonitorEnabled,
                    onCheckedChange = { vm.updateAdbMonitor(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "开发者模式警告",
            subtitle = if (settings.devModeWarning) "开发者模式开启时弹出警告" else "已关闭",
            icon = Icons.Default.WarningAmber,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.devModeWarning,
                    onCheckedChange = { vm.updateDevModeWarning(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "调试日志",
            subtitle = if (settings.debugLoggingEnabled) "输出详细调试日志" else "仅输出关键日志",
            icon = Icons.Default.BugReport,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.debugLoggingEnabled,
                    onCheckedChange = { vm.updateDebugLogging(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "模拟位置检测",
            subtitle = if (settings.mockLocationDetection) "检测是否使用了模拟位置" else "已关闭",
            icon = Icons.Default.LocationOff,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.mockLocationDetection,
                    onCheckedChange = { vm.updateMockLocationDetection(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun FileMonitorGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "文件监控") {
        GlassSettingsItem(
            label = "文件完整性检查",
            subtitle = if (settings.fileIntegrityCheck) "定期校验关键文件完整性" else "已关闭",
            icon = Icons.Default.Verified,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.fileIntegrityCheck,
                    onCheckedChange = { vm.updateFileIntegrityCheck(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "Inotify 文件监控",
            subtitle = if (settings.inotifyWatchEnabled) "实时监控文件系统事件" else "已关闭",
            icon = Icons.Default.Visibility,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.inotifyWatchEnabled,
                    onCheckedChange = { vm.updateInotifyWatch(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "系统分区监控",
            subtitle = if (settings.systemPartitionMonitor) "监控系统分区挂载状态变化" else "已关闭",
            icon = Icons.Default.Dataset,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.systemPartitionMonitor,
                    onCheckedChange = { vm.updateSystemPartitionMonitor(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "风险文件警报",
            subtitle = if (settings.criticalFileAlert) "检测到可疑系统文件变更时发出警报" else "已关闭",
            icon = Icons.Default.Warning,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.criticalFileAlert,
                    onCheckedChange = { vm.updateCriticalFileAlert(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = ErrorRed,
                        checkedTrackColor = ErrorRed.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun ProcessGroup(settings: AppSettings, vm: SettingsViewModel) {
    var whitelistExpanded by remember { mutableStateOf(false) }
    var whitelistText by remember(settings.processWhitelist) { mutableStateOf(settings.processWhitelist) }

    GlassSettingsGroup(title = "进程管理") {
        GlassSettingsItem(
            label = "可疑进程检测",
            subtitle = if (settings.suspiciousProcessDetect) "检测运行中的可疑进程" else "已关闭",
            icon = Icons.Default.PestControl,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.suspiciousProcessDetect,
                    onCheckedChange = { vm.updateSuspiciousProcessDetect(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "进程白名单",
            subtitle = if (settings.processWhitelist.isNotBlank()) "${settings.processWhitelist.split(",").size} 个进程" else "未设置",
            icon = Icons.Default.Apps,
            accentColor = AccentMint,
            onClick = { whitelistExpanded = !whitelistExpanded }
        )
        if (whitelistExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = whitelistText,
                    onValueChange = { whitelistText = it },
                    placeholder = { Text("com.example.app,com.other.process", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentMint,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateProcessWhitelist(whitelistText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentMint),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
        GlassSettingsItem(
            label = "后台进程监控",
            subtitle = if (settings.bgProcessMonitor) "监控后台运行的所有进程" else "已关闭",
            icon = Icons.Default.Memory,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.bgProcessMonitor,
                    onCheckedChange = { vm.updateBgProcessMonitor(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "进程异常警报",
            subtitle = if (settings.processAnomalyAlert) "进程行为异常时发出警报" else "已关闭",
            icon = Icons.Default.Psychology,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.processAnomalyAlert,
                    onCheckedChange = { vm.updateProcessAnomalyAlert(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun BatteryOptimizationGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "电池优化") {
        GlassSettingsItem(
            label = "电池优化白名单",
            subtitle = if (settings.batteryWhitelistEnabled) "将 APEX-Root 加入系统电池优化白名单" else "已关闭",
            icon = Icons.Default.BatteryFull,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.batteryWhitelistEnabled,
                    onCheckedChange = { vm.updateBatteryWhitelist(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "Doze 免打扰",
            subtitle = if (settings.dozeExemptEnabled) "应用不受 Doze 模式限制" else "已关闭",
            icon = Icons.Default.DarkMode,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.dozeExemptEnabled,
                    onCheckedChange = { vm.updateDozeExempt(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "Wake Lock 控制",
            subtitle = if (settings.wakeLockControl) "防止设备进入深度休眠" else "已关闭",
            icon = Icons.Default.BrightnessHigh,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.wakeLockControl,
                    onCheckedChange = { vm.updateWakeLockControl(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "省电模式覆盖",
            subtitle = if (settings.powerSaveOverride) "在系统省电模式下继续正常运行" else "受系统省电限制",
            icon = Icons.Default.EnergySavingsLeaf,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.powerSaveOverride,
                    onCheckedChange = { vm.updatePowerSaveOverride(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun PermissionGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "权限管理") {
        GlassSettingsItem(
            label = "应用权限监控",
            subtitle = if (settings.appPermissionMonitor) "监控应用申请的敏感权限" else "已关闭",
            icon = Icons.Default.Shield,
            accentColor = AccentPurple,
            trailing = {
                Switch(
                    checked = settings.appPermissionMonitor,
                    onCheckedChange = { vm.updateAppPermissionMonitor(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "权限变更警报",
            subtitle = if (settings.permissionChangeAlert) "应用权限被修改时发出警报" else "已关闭",
            icon = Icons.Default.NotificationsActive,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.permissionChangeAlert,
                    onCheckedChange = { vm.updatePermissionChangeAlert(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "可疑权限检测",
            subtitle = if (settings.suspiciousPermissionDetect) "检测应用申请的非必要敏感权限" else "已关闭",
            icon = Icons.Default.GppBad,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.suspiciousPermissionDetect,
                    onCheckedChange = { vm.updateSuspiciousPermissionDetect(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = ErrorRed,
                        checkedTrackColor = ErrorRed.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "运行时权限审计",
            subtitle = if (settings.runtimePermissionAudit) "记录应用运行时权限使用记录" else "已关闭",
            icon = Icons.Default.Assessment,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.runtimePermissionAudit,
                    onCheckedChange = { vm.updateRuntimePermissionAudit(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun SensorGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "传感器管理") {
        GlassSettingsItem(
            label = "摄像头访问监控",
            subtitle = if (settings.cameraAccessMonitor) "监控应用的摄像头调用" else "已关闭",
            icon = Icons.Default.Videocam,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.cameraAccessMonitor,
                    onCheckedChange = { vm.updateCameraAccessMonitor(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = ErrorRed,
                        checkedTrackColor = ErrorRed.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "麦克风访问监控",
            subtitle = if (settings.micAccessMonitor) "监控应用的麦克风调用" else "已关闭",
            icon = Icons.Default.Mic,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.micAccessMonitor,
                    onCheckedChange = { vm.updateMicAccessMonitor(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = ErrorRed,
                        checkedTrackColor = ErrorRed.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "传感器访问日志",
            subtitle = if (settings.sensorAccessLog) "记录所有传感器访问行为" else "已关闭",
            icon = Icons.Default.Sensors,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.sensorAccessLog,
                    onCheckedChange = { vm.updateSensorAccessLog(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "剪贴板监控",
            subtitle = if (settings.clipboardMonitorEnabled) "监控其他应用读取剪贴板" else "已关闭",
            icon = Icons.Default.ContentPaste,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.clipboardMonitorEnabled,
                    onCheckedChange = { vm.updateClipboardMonitor(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun AuditGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "审计日志") {
        GlassSettingsItem(
            label = "审计跟踪",
            subtitle = if (settings.auditTrailEnabled) "记录所有安全事件和操作" else "已关闭",
            icon = Icons.Default.History,
            accentColor = AccentPurple,
            trailing = {
                Switch(
                    checked = settings.auditTrailEnabled,
                    onCheckedChange = { vm.updateAuditTrail(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "合规模式",
            subtitle = if (settings.complianceModeEnabled) "符合安全合规标准记录" else "已关闭",
            icon = Icons.Default.Gavel,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.complianceModeEnabled,
                    onCheckedChange = { vm.updateComplianceMode(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "事件保留天数",
            subtitle = "${settings.eventRetentionDays} 天",
            icon = Icons.Default.CalendarMonth,
            accentColor = AccentBlue
        )
        Row(
            modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text("1", fontSize = 10.sp, color = TextTertiary)
            Slider(
                value = settings.eventRetentionDays.toFloat(),
                onValueChange = { vm.updateEventRetentionDays(it.toInt()) },
                valueRange = 1f..365f,
                steps = 51,
                modifier = Modifier.weight(1f).padding(horizontal = 8.dp),
                colors = SliderDefaults.colors(
                    thumbColor = AccentBlue,
                    activeTrackColor = AccentBlue,
                    inactiveTrackColor = AccentBlue.copy(alpha = 0.15f)
                )
            )
            Text("365", fontSize = 10.sp, color = TextTertiary)
        }
        GlassSettingsItem(
            label = "审计日志导出",
            subtitle = if (settings.auditExportEnabled) "允许导出审计日志" else "已关闭",
            icon = Icons.Default.FileDownload,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.auditExportEnabled,
                    onCheckedChange = { vm.updateAuditExport(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun CommunityGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "社区") {
        GlassSettingsItem(
            label = "社区论坛",
            subtitle = if (settings.communityForum) "在应用内显示社区论坛入口" else "已关闭",
            icon = Icons.Default.Forum,
            accentColor = AccentPurple,
            trailing = {
                Switch(
                    checked = settings.communityForum,
                    onCheckedChange = { vm.updateCommunityForum(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "Discord 链接",
            subtitle = if (settings.discordLink) "显示 Discord 社区入口" else "已关闭",
            icon = Icons.Default.HeadsetMic,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.discordLink,
                    onCheckedChange = { vm.updateDiscordLink(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "Telegram Bot",
            subtitle = if (settings.telegramBotEnabled) "允许通过 Telegram Bot 接收通知" else "已关闭",
            icon = Icons.Default.Send,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.telegramBotEnabled,
                    onCheckedChange = { vm.updateTelegramBot(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "GitHub 集成",
            subtitle = if (settings.githubIntegration) "在应用内显示 GitHub 仓库入口" else "已关闭",
            icon = Icons.Default.Code,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.githubIntegration,
                    onCheckedChange = { vm.updateGithubIntegration(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun ExperimentalGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "实验性功能") {
        GlassSettingsItem(
            label = "Beta 特性开关",
            subtitle = if (settings.betaFeatureFlags) "启用未正式发布的 Beta 特性" else "已关闭",
            icon = Icons.Default.Science,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.betaFeatureFlags,
                    onCheckedChange = { vm.updateBetaFeatureFlags(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "未发布模块",
            subtitle = if (settings.unreleasedModEnabled) "加载内部开发中的模块" else "已关闭",
            icon = Icons.Default.Extension,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.unreleasedModEnabled,
                    onCheckedChange = { vm.updateUnreleasedMod(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "机器学习检测",
            subtitle = if (settings.mlDetectionEnabled) "使用 ML 模型增强检测准确率" else "已关闭",
            icon = Icons.Default.Psychology,
            accentColor = AccentPurple,
            trailing = {
                Switch(
                    checked = settings.mlDetectionEnabled,
                    onCheckedChange = { vm.updateMlDetection(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "提前体验计划",
            subtitle = if (settings.earlyAccessProgram) "接收提前体验版本推送" else "已关闭",
            icon = Icons.Default.RocketLaunch,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.earlyAccessProgram,
                    onCheckedChange = { vm.updateEarlyAccessProgram(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun RootManagementGroup(settings: AppSettings, vm: SettingsViewModel) {
    var suPathExpanded by remember { mutableStateOf(false) }
    var suPathText by remember(settings.suBinaryPath) { mutableStateOf(settings.suBinaryPath) }

    GlassSettingsGroup(title = "Root 管理") {
        GlassSettingsItem(
            label = "SU 二进制路径",
            subtitle = if (settings.suBinaryPath.isNotBlank()) settings.suBinaryPath else "自动检测",
            icon = Icons.Default.Terminal,
            accentColor = AccentPurple,
            onClick = { suPathExpanded = !suPathExpanded }
        )
        if (suPathExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = suPathText,
                    onValueChange = { suPathText = it },
                    placeholder = { Text("/system/xbin/su", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentPurple,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateSuBinaryPath(suPathText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentPurple),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
        GlassSettingsItem(
            label = "Root 访问超时",
            subtitle = "${settings.rootAccessTimeout} 秒",
            icon = Icons.Default.Timer,
            accentColor = AccentGold
        )
        Row(
            modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text("5", fontSize = 10.sp, color = TextTertiary)
            Slider(
                value = settings.rootAccessTimeout.toFloat(),
                onValueChange = { vm.updateRootAccessTimeout(it.toInt()) },
                valueRange = 5f..120f,
                steps = 22,
                modifier = Modifier.weight(1f).padding(horizontal = 8.dp),
                colors = SliderDefaults.colors(
                    thumbColor = AccentGold,
                    activeTrackColor = AccentGold,
                    inactiveTrackColor = AccentGold.copy(alpha = 0.15f)
                )
            )
            Text("120", fontSize = 10.sp, color = TextTertiary)
        }
        GlassSettingsItem(
            label = "Root 权限审计",
            subtitle = if (settings.rootPermissionAudit) "记录所有 Root 权限使用记录" else "已关闭",
            icon = Icons.Default.AccountBalance,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.rootPermissionAudit,
                    onCheckedChange = { vm.updateRootPermissionAudit(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "多用户支持",
            subtitle = if (settings.multiUserSupport) "支持多用户环境下的 Root 管理" else "仅当前用户",
            icon = Icons.Default.People,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.multiUserSupport,
                    onCheckedChange = { vm.updateMultiUserSupport(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun SelinuxGroup(settings: AppSettings, vm: SettingsViewModel) {
    var contextExpanded by remember { mutableStateOf(false) }
    var contextText by remember(settings.selinuxContext) { mutableStateOf(settings.selinuxContext) }

    GlassSettingsGroup(title = "SELinux") {
        GlassSettingsItem(
            label = "SELinux 上下文",
            subtitle = if (settings.selinuxContext.isNotBlank()) settings.selinuxContext else "默认上下文",
            icon = Icons.Default.Security,
            accentColor = AccentPurple,
            onClick = { contextExpanded = !contextExpanded }
        )
        if (contextExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = contextText,
                    onValueChange = { contextText = it },
                    placeholder = { Text("u:r:init:s0", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentPurple,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateSelinuxContext(contextText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentPurple),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
        GlassSettingsItem(
            label = "SELinux 策略检查",
            subtitle = if (settings.selinuxPolicyCheck) "检查 SELinux 策略是否被篡改" else "已关闭",
            icon = Icons.Default.Policy,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.selinuxPolicyCheck,
                    onCheckedChange = { vm.updateSelinuxPolicyCheck(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "SELinux 模式切换",
            subtitle = if (settings.selinuxModeToggle) "允许在 enforcing/permissive 间切换" else "已关闭",
            icon = Icons.Default.SwapHoriz,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.selinuxModeToggle,
                    onCheckedChange = { vm.updateSelinuxModeToggle(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "上下文标签验证",
            subtitle = if (settings.contextLabelVerify) "验证文件安全上下文标签" else "已关闭",
            icon = Icons.Default.Tag,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.contextLabelVerify,
                    onCheckedChange = { vm.updateContextLabelVerify(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun NamespaceGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "命名空间") {
        GlassSettingsItem(
            label = "Mount 命名空间隔离",
            subtitle = if (settings.mountNamespaceIsolation) "隔离挂载命名空间防止检测" else "已关闭",
            icon = Icons.Default.FolderOff,
            accentColor = AccentPurple,
            trailing = {
                Switch(
                    checked = settings.mountNamespaceIsolation,
                    onCheckedChange = { vm.updateMountNamespaceIsolation(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "PID 命名空间",
            subtitle = if (settings.pidNamespace) "隔离进程命名空间" else "已关闭",
            icon = Icons.Default.Numbers,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.pidNamespace,
                    onCheckedChange = { vm.updatePidNamespace(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "网络命名空间",
            subtitle = if (settings.networkNamespace) "隔离网络命名空间" else "已关闭",
            icon = Icons.Default.Lan,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.networkNamespace,
                    onCheckedChange = { vm.updateNetworkNamespace(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "IPC 命名空间",
            subtitle = if (settings.ipcNamespace) "隔离进程间通信命名空间" else "已关闭",
            icon = Icons.Default.Hub,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.ipcNamespace,
                    onCheckedChange = { vm.updateIpcNamespace(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun ModuleGroup(settings: AppSettings, vm: SettingsViewModel) {
    var orderExpanded by remember { mutableStateOf(false) }
    var orderText by remember(settings.moduleLoadOrder) { mutableStateOf(settings.moduleLoadOrder) }
    var blacklistExpanded by remember { mutableStateOf(false) }
    var blacklistText by remember(settings.moduleBlacklist) { mutableStateOf(settings.moduleBlacklist) }

    GlassSettingsGroup(title = "模块管理") {
        GlassSettingsItem(
            label = "模块加载顺序",
            subtitle = if (settings.moduleLoadOrder.isNotBlank()) "${settings.moduleLoadOrder.split(",").size} 个模块" else "默认顺序",
            icon = Icons.Default.FormatListNumbered,
            accentColor = AccentPurple,
            onClick = { orderExpanded = !orderExpanded }
        )
        if (orderExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = orderText,
                    onValueChange = { orderText = it },
                    placeholder = { Text("module_a,module_b,module_c", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentPurple,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateModuleLoadOrder(orderText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentPurple),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
        GlassSettingsItem(
            label = "模块兼容性检查",
            subtitle = if (settings.moduleCompatCheck) "加载前检查模块兼容性" else "跳过检查",
            icon = Icons.Default.CheckCircle,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.moduleCompatCheck,
                    onCheckedChange = { vm.updateModuleCompatCheck(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "模块更新策略",
            subtitle = settings.moduleUpdatePolicy,
            icon = Icons.Default.Update,
            accentColor = AccentGold
        )
        Row(
            modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
            horizontalArrangement = Arrangement.spacedBy(6.dp)
        ) {
            listOf("stable", "beta", "canary").forEach { policy ->
                FilterChip(
                    selected = settings.moduleUpdatePolicy == policy,
                    onClick = { vm.updateModuleUpdatePolicy(policy) },
                    label = { Text(policy, fontSize = 10.sp) },
                    colors = FilterChipDefaults.filterChipColors(
                        selectedContainerColor = AccentGold.copy(alpha = 0.2f),
                        selectedLabelColor = AccentGold
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
            }
        }
        GlassSettingsItem(
            label = "模块黑名单",
            subtitle = if (settings.moduleBlacklist.isNotBlank()) "${settings.moduleBlacklist.split(",").size} 个模块" else "未设置",
            icon = Icons.Default.Block,
            accentColor = ErrorRed,
            onClick = { blacklistExpanded = !blacklistExpanded }
        )
        if (blacklistExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = blacklistText,
                    onValueChange = { blacklistText = it },
                    placeholder = { Text("bad_module,malicious_mod", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = ErrorRed,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateModuleBlacklist(blacklistText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = ErrorRed),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
    }
}

@Composable
private fun ReportGroup(settings: AppSettings, vm: SettingsViewModel) {
    var detailExpanded by remember { mutableStateOf(false) }
    var detailText by remember(settings.reportDetailLevel) { mutableStateOf(settings.reportDetailLevel) }
    var formatExpanded by remember { mutableStateOf(false) }
    var formatText by remember(settings.reportFormatDetailed) { mutableStateOf(settings.reportFormatDetailed) }

    GlassSettingsGroup(title = "报告设置") {
        GlassSettingsItem(
            label = "报告详细级别",
            subtitle = settings.reportDetailLevel,
            icon = Icons.Default.Tune,
            accentColor = AccentPurple,
            onClick = { detailExpanded = !detailExpanded }
        )
        if (detailExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                horizontalArrangement = Arrangement.spacedBy(6.dp)
            ) {
                listOf("basic", "standard", "detailed").forEach { level ->
                    FilterChip(
                        selected = settings.reportDetailLevel == level,
                        onClick = { vm.updateReportDetailLevel(level) },
                        label = { Text(level, fontSize = 10.sp) },
                        colors = FilterChipDefaults.filterChipColors(
                            selectedContainerColor = AccentPurple.copy(alpha = 0.2f),
                            selectedLabelColor = AccentPurple
                        ),
                        shape = RoundedCornerShape(10.dp)
                    )
                }
            }
        }
        GlassSettingsItem(
            label = "报告格式",
            subtitle = settings.reportFormatDetailed.uppercase(),
            icon = Icons.Default.Description,
            accentColor = AccentBlue,
            onClick = { formatExpanded = !formatExpanded }
        )
        if (formatExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = formatText,
                    onValueChange = { formatText = it },
                    placeholder = { Text("pdf / html / txt", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentBlue,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateReportFormatDetailed(formatText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentBlue),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
        GlassSettingsItem(
            label = "自动分享报告",
            subtitle = if (settings.autoReportShare) "生成报告后自动打开分享" else "已关闭",
            icon = Icons.Default.Share,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.autoReportShare,
                    onCheckedChange = { vm.updateAutoReportShare(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "报告加密",
            subtitle = if (settings.reportEncryption) "使用 AES-256 加密报告文件" else "不加密",
            icon = Icons.Default.Lock,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.reportEncryption,
                    onCheckedChange = { vm.updateReportEncryption(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun SecureInputGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "安全输入") {
        GlassSettingsItem(
            label = "安全键盘",
            subtitle = if (settings.secureKeyboard) "使用内置安全键盘输入敏感信息" else "系统键盘",
            icon = Icons.Default.Keyboard,
            accentColor = AccentPurple,
            trailing = {
                Switch(
                    checked = settings.secureKeyboard,
                    onCheckedChange = { vm.updateSecureKeyboard(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "输入混淆",
            subtitle = if (settings.inputObfuscation) "混淆输入数据防止侧信道窃取" else "已关闭",
            icon = Icons.Default.Blind,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.inputObfuscation,
                    onCheckedChange = { vm.updateInputObfuscation(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "屏幕覆盖保护",
            subtitle = if (settings.overlayProtection) "检测并阻止屏幕覆盖层攻击" else "已关闭",
            icon = Icons.Default.Layers,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.overlayProtection,
                    onCheckedChange = { vm.updateOverlayProtection(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "键盘记录检测",
            subtitle = if (settings.keyloggingDetect) "检测键盘记录器等恶意软件" else "已关闭",
            icon = Icons.Default.KeyboardHide,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.keyloggingDetect,
                    onCheckedChange = { vm.updateKeyloggingDetect(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun WirelessGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "无线安全") {
        GlassSettingsItem(
            label = "WiFi 热点检测",
            subtitle = if (settings.wifiHotspotDetect) "设备开启热点时发出安全提醒" else "已关闭",
            icon = Icons.Default.WifiTethering,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.wifiHotspotDetect,
                    onCheckedChange = { vm.updateWifiHotspotDetect(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "蓝牙网络共享警报",
            subtitle = if (settings.bluetoothTetherAlert) "蓝牙网络共享开启时发出警报" else "已关闭",
            icon = Icons.Default.BluetoothConnected,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.bluetoothTetherAlert,
                    onCheckedChange = { vm.updateBluetoothTetherAlert(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "NFC 检测",
            subtitle = if (settings.nfcDetection) "监控 NFC 标签读取和写入" else "已关闭",
            icon = Icons.Default.Nfc,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.nfcDetection,
                    onCheckedChange = { vm.updateNfcDetection(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "飞行模式行为",
            subtitle = if (settings.airplaneModeBehavior) "飞行模式开启时自动暂停检测" else "不受影响",
            icon = Icons.Default.AirplanemodeActive,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.airplaneModeBehavior,
                    onCheckedChange = { vm.updateAirplaneModeBehavior(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun EmergencyCommGroup(settings: AppSettings, vm: SettingsViewModel) {
    var smsExpanded by remember { mutableStateOf(false) }
    var smsText by remember(settings.emergencySmsTemplate) { mutableStateOf(settings.emergencySmsTemplate) }

    GlassSettingsGroup(title = "应急通信") {
        GlassSettingsItem(
            label = "应急短信模板",
            subtitle = if (settings.emergencySmsTemplate.isNotBlank()) "已设置" else "未设置",
            icon = Icons.Default.Sms,
            accentColor = AccentPurple,
            onClick = { smsExpanded = !smsExpanded }
        )
        if (smsExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = smsText,
                    onValueChange = { smsText = it },
                    placeholder = { Text("[ALERT] Device compromised at {time}", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentPurple,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateEmergencySmsTemplate(smsText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentPurple),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
        GlassSettingsItem(
            label = "紧急呼叫",
            subtitle = if (settings.emergencyCall) "检测到风险时自动拨打紧急号码" else "已关闭",
            icon = Icons.Default.Phone,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.emergencyCall,
                    onCheckedChange = { vm.updateEmergencyCall(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = ErrorRed,
                        checkedTrackColor = ErrorRed.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "位置共享",
            subtitle = if (settings.locationSharing) "紧急情况下共享设备位置" else "已关闭",
            icon = Icons.Default.LocationOn,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.locationSharing,
                    onCheckedChange = { vm.updateLocationSharing(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "心跳检测",
            subtitle = if (settings.heartbeatPing) "定期发送心跳包检测设备在线状态" else "已关闭",
            icon = Icons.Default.Favorite,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.heartbeatPing,
                    onCheckedChange = { vm.updateHeartbeatPing(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun IntegrityGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "Play Integrity") {
        GlassSettingsItem(
            label = "Play Integrity 检测",
            subtitle = if (settings.playIntegrityCheck) "检查设备 Play Integrity 状态" else "已关闭",
            icon = Icons.Default.Verified,
            accentColor = AccentPurple,
            trailing = {
                Switch(
                    checked = settings.playIntegrityCheck,
                    onCheckedChange = { vm.updatePlayIntegrityCheck(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "密钥认证",
            subtitle = if (settings.keyAttestationEnabled) "使用硬件密钥认证设备身份" else "已关闭",
            icon = Icons.Default.Key,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.keyAttestationEnabled,
                    onCheckedChange = { vm.updateKeyAttestation(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "CTS 配置文件匹配",
            subtitle = if (settings.ctsProfileMatch) "检查 CTS 配置文件是否匹配" else "已关闭",
            icon = Icons.Default.Fingerprint,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.ctsProfileMatch,
                    onCheckedChange = { vm.updateCtsProfileMatch(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "基础完整性",
            subtitle = if (settings.basicIntegrity) "验证系统基本完整性" else "已关闭",
            icon = Icons.Default.Shield,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.basicIntegrity,
                    onCheckedChange = { vm.updateBasicIntegrity(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun VpnGroup(settings: AppSettings, vm: SettingsViewModel) {
    var proxyExpanded by remember { mutableStateOf(false) }
    var proxyText by remember(settings.vpnProxyConfig) { mutableStateOf(settings.vpnProxyConfig) }

    GlassSettingsGroup(title = "VPN 管理") {
        GlassSettingsItem(
            label = "始终开启 VPN",
            subtitle = if (settings.alwaysOnVpn) "系统始终保持 VPN 连接" else "已关闭",
            icon = Icons.Default.VpnLock,
            accentColor = AccentPurple,
            trailing = {
                Switch(
                    checked = settings.alwaysOnVpn,
                    onCheckedChange = { vm.updateAlwaysOnVpn(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "VPN 断线保护",
            subtitle = if (settings.vpnKillSwitch) "VPN 断开时阻止网络流量" else "已关闭",
            icon = Icons.Default.NetworkCheck,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.vpnKillSwitch,
                    onCheckedChange = { vm.updateVpnKillSwitch(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = ErrorRed,
                        checkedTrackColor = ErrorRed.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "DNS 泄漏防护",
            subtitle = if (settings.dnsLeakProtection) "防止 DNS 请求泄露真实 IP" else "已关闭",
            icon = Icons.Default.Dns,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.dnsLeakProtection,
                    onCheckedChange = { vm.updateDnsLeakProtection(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "Split Tunneling",
            subtitle = if (settings.splitTunneling) "部分应用绕过 VPN" else "全部流量走 VPN",
            icon = Icons.Default.CallSplit,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.splitTunneling,
                    onCheckedChange = { vm.updateSplitTunneling(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "代理配置",
            subtitle = if (settings.vpnProxyConfig.isNotBlank()) "已配置" else "未设置",
            icon = Icons.Default.Http,
            accentColor = AccentMint,
            onClick = { proxyExpanded = !proxyExpanded }
        )
        if (proxyExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = proxyText,
                    onValueChange = { proxyText = it },
                    placeholder = { Text("proxy.example.com:8080", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentMint,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateVpnProxyConfig(proxyText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentMint),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
    }
}

@Composable
private fun CertGroup(settings: AppSettings, vm: SettingsViewModel) {
    var caExpanded by remember { mutableStateOf(false) }
    var caText by remember(settings.trustedCaList) { mutableStateOf(settings.trustedCaList) }

    GlassSettingsGroup(title = "证书管理") {
        GlassSettingsItem(
            label = "证书锁定",
            subtitle = if (settings.certPinningEnabled) "锁定可信证书防 MITM 攻击" else "已关闭",
            icon = Icons.Default.Lock,
            accentColor = AccentPurple,
            trailing = {
                Switch(
                    checked = settings.certPinningEnabled,
                    onCheckedChange = { vm.updateCertPinning(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "SSL 验证级别",
            subtitle = settings.sslVerificationLevel,
            icon = Icons.Default.VerifiedUser,
            accentColor = AccentGold
        )
        Row(
            modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
            horizontalArrangement = Arrangement.spacedBy(6.dp)
        ) {
            listOf("none", "standard", "strict").forEach { level ->
                FilterChip(
                    selected = settings.sslVerificationLevel == level,
                    onClick = { vm.updateSslVerificationLevel(level) },
                    label = { Text(level, fontSize = 10.sp) },
                    colors = FilterChipDefaults.filterChipColors(
                        selectedContainerColor = AccentGold.copy(alpha = 0.2f),
                        selectedLabelColor = AccentGold
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
            }
        }
        GlassSettingsItem(
            label = "可信 CA 列表",
            subtitle = if (settings.trustedCaList.isNotBlank()) "${settings.trustedCaList.split(",").size} 个 CA" else "系统默认",
            icon = Icons.Default.AccountTree,
            accentColor = AccentBlue,
            onClick = { caExpanded = !caExpanded }
        )
        if (caExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = caText,
                    onValueChange = { caText = it },
                    placeholder = { Text("CA_1,CA_2,CA_3", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentBlue,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateTrustedCaList(caText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentBlue),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
        GlassSettingsItem(
            label = "证书透明度",
            subtitle = if (settings.certTransparency) "验证证书是否在透明度日志中" else "已关闭",
            icon = Icons.Default.Public,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.certTransparency,
                    onCheckedChange = { vm.updateCertTransparency(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun MagiskGroup(settings: AppSettings, vm: SettingsViewModel) {
    var hideListExpanded by remember { mutableStateOf(false) }
    var hideListText by remember(settings.magiskHideList) { mutableStateOf(settings.magiskHideList) }

    GlassSettingsGroup(title = "Magisk 管理") {
        GlassSettingsItem(
            label = "Magisk 版本检测",
            subtitle = if (settings.magiskVersionCheck) "检查 Magisk 版本及更新" else "已关闭",
            icon = Icons.Default.Info,
            accentColor = AccentPurple,
            trailing = {
                Switch(
                    checked = settings.magiskVersionCheck,
                    onCheckedChange = { vm.updateMagiskVersionCheck(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "Magisk 模块扫描",
            subtitle = if (settings.magiskModuleScan) "扫描已安装的 Magisk 模块" else "已关闭",
            icon = Icons.Default.Extension,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.magiskModuleScan,
                    onCheckedChange = { vm.updateMagiskModuleScan(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "Zygisk 检测",
            subtitle = if (settings.zygiskDetect) "检测 Zygisk 是否处于活动状态" else "已关闭",
            icon = Icons.Default. Psychology,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.zygiskDetect,
                    onCheckedChange = { vm.updateZygiskDetect(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "Magisk Hide 列表",
            subtitle = if (settings.magiskHideList.isNotBlank()) "${settings.magiskHideList.split(",").size} 个应用" else "未设置",
            icon = Icons.Default.VisibilityOff,
            accentColor = AccentMint,
            onClick = { hideListExpanded = !hideListExpanded }
        )
        if (hideListExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = hideListText,
                    onValueChange = { hideListText = it },
                    placeholder = { Text("com.bank.app,com.game.app", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentMint,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateMagiskHideList(hideListText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentMint),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
    }
}

@Composable
private fun XposedGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "Xposed 检测") {
        GlassSettingsItem(
            label = "Xposed 框架检测",
            subtitle = if (settings.xposedDetectEnabled) "检测 Xposed 框架是否已安装" else "已关闭",
            icon = Icons.Default.Block,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.xposedDetectEnabled,
                    onCheckedChange = { vm.updateXposedDetect(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = ErrorRed,
                        checkedTrackColor = ErrorRed.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "Xposed 模块扫描",
            subtitle = if (settings.xposedModuleScan) "扫描已激活的 Xposed 模块" else "已关闭",
            icon = Icons.Default.Search,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.xposedModuleScan,
                    onCheckedChange = { vm.updateXposedModuleScan(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "LSPosed 检测",
            subtitle = if (settings.lsposedDetectEnabled) "检测 LSPosed 框架" else "已关闭",
            icon = Icons.Default. GppBad,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.lsposedDetectEnabled,
                    onCheckedChange = { vm.updateLsposedDetect(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = ErrorRed,
                        checkedTrackColor = ErrorRed.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "Riru 检测",
            subtitle = if (settings.riruDetectEnabled) "检测 Riru 框架" else "已关闭",
            icon = Icons.Default. Warning,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.riruDetectEnabled,
                    onCheckedChange = { vm.updateRiruDetect(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun ScriptGroup(settings: AppSettings, vm: SettingsViewModel) {
    var initScriptsExpanded by remember { mutableStateOf(false) }
    var initScriptsText by remember(settings.customInitScripts) { mutableStateOf(settings.customInitScripts) }
    var bootScriptsExpanded by remember { mutableStateOf(false) }
    var bootScriptsText by remember(settings.postBootScripts) { mutableStateOf(settings.postBootScripts) }

    GlassSettingsGroup(title = "脚本管理") {
        GlassSettingsItem(
            label = "自定义初始化脚本",
            subtitle = if (settings.customInitScripts.isNotBlank()) "已配置" else "未设置",
            icon = Icons.Default.Terminal,
            accentColor = AccentPurple,
            onClick = { initScriptsExpanded = !initScriptsExpanded }
        )
        if (initScriptsExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = initScriptsText,
                    onValueChange = { initScriptsText = it },
                    placeholder = { Text("/data/scripts/init.sh", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentPurple,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updateCustomInitScripts(initScriptsText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentPurple),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
        GlassSettingsItem(
            label = "启动后脚本",
            subtitle = if (settings.postBootScripts.isNotBlank()) "已配置" else "未设置",
            icon = Icons.Default.PowerSettingsNew,
            accentColor = AccentBlue,
            onClick = { bootScriptsExpanded = !bootScriptsExpanded }
        )
        if (bootScriptsExpanded) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedTextField(
                    value = bootScriptsText,
                    onValueChange = { bootScriptsText = it },
                    placeholder = { Text("/data/scripts/post_boot.sh", fontSize = 11.sp) },
                    modifier = Modifier.weight(1f),
                    singleLine = true,
                    textStyle = MaterialTheme.typography.bodySmall,
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentBlue,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = { vm.updatePostBootScripts(bootScriptsText) },
                    shape = RoundedCornerShape(10.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentBlue),
                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                ) { Text("保存", fontSize = 11.sp) }
            }
        }
        GlassSettingsItem(
            label = "定时执行脚本",
            subtitle = if (settings.scriptRunOnSchedule) "按设定计划执行脚本" else "手动执行",
            icon = Icons.Default.Schedule,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.scriptRunOnSchedule,
                    onCheckedChange = { vm.updateScriptRunOnSchedule(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "脚本日志输出",
            subtitle = if (settings.scriptLogOutput) "记录脚本执行输出日志" else "不记录",
            icon = Icons.Default.Article,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.scriptLogOutput,
                    onCheckedChange = { vm.updateScriptLogOutput(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun HardwareGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "硬件认证") {
        GlassSettingsItem(
            label = "硬件认证",
            subtitle = if (settings.hardwareAttestation) "使用硬件支持的安全认证" else "已关闭",
            icon = Icons.Default.VerifiedUser,
            accentColor = AccentPurple,
            trailing = {
                Switch(
                    checked = settings.hardwareAttestation,
                    onCheckedChange = { vm.updateHardwareAttestation(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "TEE 检查",
            subtitle = if (settings.teeCheckEnabled) "验证可信执行环境状态" else "已关闭",
            icon = Icons.Default.Memory,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.teeCheckEnabled,
                    onCheckedChange = { vm.updateTeeCheck(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "安全元件检查",
            subtitle = if (settings.secureElementCheck) "检测安全元件 (eSE) 状态" else "已关闭",
            icon = Icons.Default.SimCard,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.secureElementCheck,
                    onCheckedChange = { vm.updateSecureElementCheck(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "指纹硬件检测",
            subtitle = if (settings.fingerprintHardwareCheck) "验证指纹硬件是否被篡改" else "已关闭",
            icon = Icons.Default.Fingerprint,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.fingerprintHardwareCheck,
                    onCheckedChange = { vm.updateFingerprintHardwareCheck(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun SystemIntegrityGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "系统完整性") {
        GlassSettingsItem(
            label = "dm-verity 检查",
            subtitle = if (settings.dmVerityCheck) "验证设备映射器完整性" else "已关闭",
            icon = Icons.Default.Verified,
            accentColor = AccentPurple,
            trailing = {
                Switch(
                    checked = settings.dmVerityCheck,
                    onCheckedChange = { vm.updateDmVerityCheck(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "AVB 检查",
            subtitle = if (settings.avbCheck) "验证 Android Verified Boot 状态" else "已关闭",
            icon = Icons.Default.Shield,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.avbCheck,
                    onCheckedChange = { vm.updateAvbCheck(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "系统分区只读",
            subtitle = if (settings.systemPartitionRo) "验证系统分区是否为只读" else "已关闭",
            icon = Icons.Default.FolderOff,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.systemPartitionRo,
                    onCheckedChange = { vm.updateSystemPartitionRo(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "VBMeta 检查",
            subtitle = if (settings.vbmetaCheck) "验证 VBMeta 完整性" else "已关闭",
            icon = Icons.Default.Dataset,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.vbmetaCheck,
                    onCheckedChange = { vm.updateVbmetaCheck(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun AboutGroup(
    apexViewModel: ApexViewModel? = null,
    onNavigateToLogs: (() -> Unit)? = null
) {
    GlassSettingsGroup(title = "关于") {
        GlassSettingsItem(
            label = "版本信息",
            subtitle = "APEX-Root v1.0.2",
            icon = Icons.Default.Info,
            accentColor = AccentPurple
        )
        GlassSettingsItem(
            label = "导出报告",
            subtitle = "分享或保存检测报告",
            icon = Icons.Default.Share,
            accentColor = AccentBlue,
            onClick = { apexViewModel?.triggerExport() }
        )
        GlassSettingsItem(
            label = "导出诊断日志",
            subtitle = "导出运行日志用于故障排查",
            icon = Icons.Default.BugReport,
            accentColor = ErrorRed,
            onClick = { onNavigateToLogs?.invoke() }
        )
        GlassSettingsItem(
            label = "开源许可",
            subtitle = "查看第三方开源组件许可",
            icon = Icons.Default.Description,
            accentColor = TextTertiary
        )
    }
}

@Composable
private fun PrivacyProtectionGroup(settings: AppSettings, vm: SettingsViewModel) {
    GlassSettingsGroup(title = "隐私保护") {
        GlassSettingsItem(
            label = "屏幕录制检测",
            subtitle = if (settings.screenRecordDetect) "检测屏幕录制行为并告警" else "已关闭",
            icon = Icons.Default.Videocam,
            accentColor = AccentPurple,
            trailing = {
                Switch(
                    checked = settings.screenRecordDetect,
                    onCheckedChange = { vm.updateScreenRecordDetect(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "VPN / 代理检测",
            subtitle = if (settings.vpnDetectEnabled) "检测 VPN 及代理连接" else "已关闭",
            icon = Icons.Default.VpnLock,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.vpnDetectEnabled,
                    onCheckedChange = { vm.updateVpnDetectEnabled(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "截图拦截",
            subtitle = if (settings.screenshotBlock) "禁止截取应用界面" else "允许截图",
            icon = Icons.Default.PhotoCamera,
            accentColor = AccentGold,
            trailing = {
                Switch(
                    checked = settings.screenshotBlock,
                    onCheckedChange = { vm.updateScreenshotBlock(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}

@Composable
private fun NetworkSecurityGroup(settings: AppSettings, vm: SettingsViewModel) {
    var dohExpanded by remember { mutableStateOf(false) }
    val dohProviders = listOf("cloudflare", "google", "quad9", "aliyun")

    GlassSettingsGroup(title = "网络安全") {
        GlassSettingsItem(
            label = "DNS over HTTPS",
            subtitle = if (settings.dnsOverHttps) "加密 DNS 解析" else "标准 DNS",
            icon = Icons.Default.Dns,
            accentColor = AccentBlue,
            trailing = {
                Switch(
                    checked = settings.dnsOverHttps,
                    onCheckedChange = { vm.updateDnsOverHttps(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        if (settings.dnsOverHttps) {
            GlassSettingsItem(
                label = "DoH 提供商",
                subtitle = settings.dnsOverHttpsProvider.replaceFirstChar { it.uppercase() },
                icon = Icons.Default.Dns,
                accentColor = AccentGold,
                onClick = { dohExpanded = !dohExpanded }
            )
            if (dohExpanded) {
                Column(modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 4.dp)) {
                    dohProviders.forEach { provider ->
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .clickable { vm.updateDnsOverHttpsProvider(provider); dohExpanded = false }
                                .padding(vertical = 10.dp, horizontal = 8.dp),
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            RadioButton(
                                selected = settings.dnsOverHttpsProvider == provider,
                                onClick = { vm.updateDnsOverHttpsProvider(provider); dohExpanded = false },
                                colors = RadioButtonDefaults.colors(selectedColor = AccentPurple)
                            )
                            Spacer(Modifier.width(8.dp))
                            Text(
                                provider.replaceFirstChar { it.uppercase() },
                                color = TextPrimary,
                                fontSize = 14.sp
                            )
                        }
                    }
                }
            }
        }
        GlassSettingsItem(
            label = "ARP 欺骗检测",
            subtitle = if (settings.arpSpoofDetect) "检测局域网 ARP 攻击" else "已关闭",
            icon = Icons.Default.Router,
            accentColor = ErrorRed,
            trailing = {
                Switch(
                    checked = settings.arpSpoofDetect,
                    onCheckedChange = { vm.updateArpSpoofDetect(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
        GlassSettingsItem(
            label = "MAC 随机化",
            subtitle = if (settings.macRandomization) "WiFi 扫描使用随机 MAC" else "真实 MAC",
            icon = Icons.Default.NetworkCheck,
            accentColor = AccentMint,
            trailing = {
                Switch(
                    checked = settings.macRandomization,
                    onCheckedChange = { vm.updateMacRandomization(it) },
                    colors = SwitchDefaults.colors(
                        checkedThumbColor = AccentPurple,
                        checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                    )
                )
            }
        )
    }
}
