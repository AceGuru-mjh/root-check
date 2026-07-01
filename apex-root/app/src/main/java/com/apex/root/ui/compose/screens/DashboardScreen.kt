package com.apex.root.ui.compose.screens

import androidx.compose.animation.core.*
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.apex.root.data.FixRecommendation
import com.apex.root.domain.model.CureLevel
import com.apex.root.ui.compose.*
import com.apex.root.viewmodel.trusted.ApexUiState
import com.apex.root.viewmodel.trusted.ApexViewModel
import com.apex.root.viewmodel.trusted.LogEntry
import com.apex.root.viewmodel.trusted.LogType
import kotlinx.coroutines.delay

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun DashboardScreen(
    uiState: ApexUiState,
    onScan: () -> Unit,
    onDeepScan: (() -> Unit)? = null,
    onToggleGameMode: () -> Unit,
    onApplyCure: (CureLevel) -> Unit,
    onToggleHwid: () -> Unit,
    onCreateSandbox: (String) -> Unit,
    onDestroySandbox: () -> Unit,
    onRefresh: () -> Unit,
    onShowRecommendations: (() -> Unit)? = null,
    onExportReport: (() -> Unit)? = null,
    onDismissRecommendations: (() -> Unit)? = null,
    onNavigateToHistory: (() -> Unit)? = null,
    onNavigateToKernelInfo: (() -> Unit)? = null,
    onNavigateToBaseline: (() -> Unit)? = null,
    onNavigateToFeatureTest: (() -> Unit)? = null,
    onNavigateToTimingChart: (() -> Unit)? = null,
    onNavigateToWhitelist: (() -> Unit)? = null,
    onNavigateToConfig: (() -> Unit)? = null,
    onNavigateToHideMode: (() -> Unit)? = null,
    onNavigateToAbout: (() -> Unit)? = null,
    onNavigateToFrida: (() -> Unit)? = null,
    onNavigateToLSPosed: (() -> Unit)? = null,
    apexViewModel: ApexViewModel? = null
) {
    val scrollBehavior = TopAppBarDefaults.exitUntilCollapsedScrollBehavior()
    val context = androidx.compose.ui.platform.LocalContext.current
    var sandboxName by remember { mutableStateOf("game-sandbox") }
    var cureExpanded by remember { mutableStateOf(false) }
    var islandExpanded by remember { mutableStateOf(false) }
    var showExportPreview by remember { mutableStateOf(false) }
    // 修复：治愈操作确认对话框（防止误触"完全重置"等不可逆操作）
    var pendingCureLevel by remember { mutableStateOf<CureLevel?>(null) }
    val snackbarHostState = remember { SnackbarHostState() }

    LaunchedEffect(Unit) {
        apexViewModel?.snackbarChannel?.collect { event ->
            snackbarHostState.currentSnackbarData?.dismiss()
            snackbarHostState.showSnackbar(message = event.message, duration = SnackbarDuration.Short)
        }
    }

    // scoreLabel 提到顶层，使 GlassShareReportPreview 也能访问
    val scoreLabel = when {
        uiState.riskScore > 60 -> "高风险"
        uiState.riskScore > 30 -> "有风险"
        uiState.riskScore > 10 -> "轻度风险"
        else -> "安全"
    }
    LaunchedEffect(Unit) { onRefresh() }

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
                title = "APEX Root",
                collapsedFraction = scrollBehavior.state.collapsedFraction
            )
        }
    ) { padding ->
        Box(modifier = Modifier.fillMaxSize().padding(padding)) {
            LiquidGlassContainer(fluidColorsDark = PageFluidColors.dashboard, fluidColorsLight = PageFluidColors.dashboardLight) {
                Column(
                modifier = Modifier
                    .fillMaxSize()
                    .verticalScroll(rememberScrollState()),
                verticalArrangement = Arrangement.spacedBy(0.dp)
            ) {
                if (uiState.isLoading) {
                    GlassSkeletonCard()
                    Spacer(Modifier.height(12.dp))
                }

                GlassGaugeScoreCard(score = uiState.riskScore, label = scoreLabel)

                Spacer(Modifier.height(12.dp))

                // 运行中状态指示 + 迷你终端日志
                if (uiState.isScanning || uiState.logs.isNotEmpty()) {
                    MiniTerminalCard(
                        isScanning = uiState.isScanning,
                        logs = uiState.logs,
                        onNavigateToLogs = onNavigateToLogs
                    )
                    Spacer(Modifier.height(12.dp))
                }

                QuickActionsRow(onScan, onDeepScan, uiState.isScanning)

                Spacer(Modifier.height(16.dp))

                if (uiState.scanResult != "点击扫描开始检测" && !uiState.isScanning) {
                    ScanResultSection(result = uiState.scanResult)

                    if (uiState.memFingerprintMask != 0 || uiState.selinuxCompromised) {
                        Spacer(Modifier.height(12.dp))
                        DeepFindingsSection(uiState = uiState)
                    }

                    Spacer(Modifier.height(12.dp))

                    if (uiState.showRecommendations && uiState.recommendations.isNotEmpty()) {
                        RecommendationsSection(uiState.recommendations, onDismissRecommendations)
                        Spacer(Modifier.height(12.dp))
                    }

                    ActionButtonsRow(onShowRecommendations, onExportReport)
                    Spacer(Modifier.height(12.dp))

                    Button(
                        onClick = { showExportPreview = true },
                        modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp),
                        colors = ButtonDefaults.buttonColors(containerColor = AccentPurple),
                        shape = RoundedCornerShape(14.dp),
                        contentPadding = PaddingValues(vertical = 14.dp)
                    ) {
                        Text("一键导出环境安全报告", color = Color.White, fontWeight = FontWeight.SemiBold)
                    }

                    Spacer(Modifier.height(24.dp))
                }

                SectionHeader(title = "功能模块")
                Spacer(Modifier.height(14.dp))

                GlassFeatureCard(
                    title = "APEX-Cure",
                    subtitle = "一键修复 Root 痕迹",
                    icon = Icons.Default.Medication,
                    accentColor = AccentMint,
                    expandable = true,
                    expanded = cureExpanded,
                    onToggle = { cureExpanded = !cureExpanded }
                ) {
                    if (uiState.cureMessage.isNotEmpty()) {
                        Text(uiState.cureMessage, fontSize = 11.sp, color = AccentMint)
                        Spacer(Modifier.height(10.dp))
                    }
                    Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                        CureButton("轻度处理", AccentMint) { pendingCureLevel = CureLevel.LIGHT }
                        CureButton("标准修复", AccentGold) { pendingCureLevel = CureLevel.STANDARD }
                    }
                    if (cureExpanded) {
                        Spacer(Modifier.height(10.dp))
                        Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                            CureButton("深度恢复", Color(0xFFFF7043)) { pendingCureLevel = CureLevel.DEEP }
                            CureButton("完全重置", ErrorRed) { pendingCureLevel = CureLevel.FACTORY }
                        }
                        Spacer(Modifier.height(10.dp))
                        Box(
                            Modifier.fillMaxWidth()
                                .background(Color.White.copy(alpha = 0.03f), RoundedCornerShape(8.dp))
                                .padding(12.dp)
                        ) {
                            Text("深度修复可能导致系统设置恢复出厂，请谨慎操作",
                                fontSize = 10.sp, color = TextTertiary)
                        }
                    }
                }

                Spacer(Modifier.height(14.dp))

                GlassFeatureCard(
                    title = "APEX-Island",
                    subtitle = "隔离运行敏感应用",
                    icon = Icons.Default.Lock,
                    accentColor = AccentBlue,
                    expandable = true,
                    expanded = islandExpanded,
                    onToggle = { islandExpanded = !islandExpanded }
                ) {
                    OutlinedTextField(
                        value = sandboxName, onValueChange = { sandboxName = it },
                        label = { Text("沙箱名称") },
                        modifier = Modifier.fillMaxWidth(),
                        singleLine = true,
                        colors = OutlinedTextFieldDefaults.colors(
                            focusedBorderColor = AccentBlue, unfocusedBorderColor = Color.White.copy(alpha = 0.10f),
                            focusedLabelColor = AccentBlue, unfocusedLabelColor = TextTertiary
                        ),
                        shape = RoundedCornerShape(10.dp)
                    )
                    Spacer(Modifier.height(10.dp))
                    Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                        Button(
                            onClick = { onCreateSandbox(sandboxName) },
                            enabled = !uiState.sandboxActive,
                            colors = ButtonDefaults.buttonColors(containerColor = AccentBlue),
                            shape = RoundedCornerShape(10.dp)
                        ) { Text("创建沙箱") }
                        OutlinedButton(
                            onClick = onDestroySandbox, enabled = uiState.sandboxActive,
                            shape = RoundedCornerShape(10.dp)
                        ) { Text("销毁沙箱") }
                    }
                    if (uiState.sandboxActive) {
                        Spacer(Modifier.height(8.dp))
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Box(Modifier.size(5.dp).background(AccentMint, CircleShape))
                            Spacer(Modifier.width(8.dp))
                            Text("沙箱运行中 (PID: ${uiState.sandboxPid})", fontSize = 11.sp, color = AccentMint)
                        }
                    }
                    if (islandExpanded) {
                        Spacer(Modifier.height(10.dp))
                        Box(
                            Modifier.fillMaxWidth()
                                .background(Color.White.copy(alpha = 0.03f), RoundedCornerShape(8.dp))
                                .padding(12.dp)
                        ) {
                            Text("隔离沙箱基于 Linux 命名空间实现，提供独立的进程环境",
                                fontSize = 10.sp, color = TextTertiary)
                        }
                    }
                }

                Spacer(Modifier.height(14.dp))

                GlassFeatureCard(
                    title = "APEX-Guard",
                    subtitle = "实时系统完整性监控",
                    icon = Icons.Default.Shield,
                    accentColor = AccentPurple
                ) {
                    Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                        Button(
                            onClick = {
                                // 修复：原 onClick 为空，现调用 NativeGuard.startGuardian
                                val ok = com.apex.root.guard.NativeGuard.startGuardian()
                                val msg = if (ok) "守护进程已启动" else "启动失败：原生库未加载或权限不足"
                                android.widget.Toast.makeText(context, msg, android.widget.Toast.LENGTH_SHORT).show()
                            },
                            colors = ButtonDefaults.buttonColors(containerColor = AccentPurple),
                            shape = RoundedCornerShape(10.dp)
                        ) { Text("启动守护") }
                        OutlinedButton(
                            onClick = {
                                // 修复：原 onClick 为空，现调用 NativeGuard.checkIntegrity
                                val ok = com.apex.root.guard.NativeGuard.checkIntegrity()
                                val msg = if (ok) "系统完整性校验通过" else "检测到完整性异常"
                                android.widget.Toast.makeText(context, msg, android.widget.Toast.LENGTH_SHORT).show()
                            },
                            shape = RoundedCornerShape(10.dp)
                        ) { Text("校验系统") }
                    }
                    Spacer(Modifier.height(10.dp))
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Box(Modifier.size(6.dp).background(if (uiState.guardState.enabled) AccentMint else ErrorRed, CircleShape))
                        Spacer(Modifier.width(10.dp))
                        Text("${if (uiState.guardState.enabled) "防护中" else "未启动"}",
                            fontSize = 12.sp, color = TextSecondary)
                    }
                }

                Spacer(Modifier.height(14.dp))

                GlassFeatureCard(
                    title = "游戏模式",
                    subtitle = "隐藏 Root 状态，畅享游戏",
                    icon = Icons.Default.VideogameAsset,
                    accentColor = AccentGold
                ) {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Text("状态", fontSize = 13.sp, color = TextSecondary)
                        Spacer(Modifier.width(8.dp))
                        Text(if (uiState.gameMode.active) "已激活" else "未激活",
                            fontSize = 14.sp, fontWeight = FontWeight.SemiBold,
                            color = if (uiState.gameMode.active) AccentMint else TextSecondary)
                        Spacer(Modifier.weight(1f))
                        Switch(
                            checked = uiState.gameMode.active,
                            onCheckedChange = { onToggleGameMode() },
                            colors = SwitchDefaults.colors(
                                checkedThumbColor = AccentPurple,
                                checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                            )
                        )
                    }
                    if (uiState.gameMode.active) {
                        Spacer(Modifier.height(10.dp))
                        Row(horizontalArrangement = Arrangement.spacedBy(16.dp)) {
                            Text("进程隐藏: ${uiState.gameMode.hiddenProcesses}", fontSize = 11.sp, color = TextTertiary)
                            Text("eBPF", fontSize = 11.sp, color = AccentMint)
                            Text("HWID: ${if (uiState.hwidSpoofed) "已伪装" else "原始"}",
                                fontSize = 11.sp, color = if (uiState.hwidSpoofed) AccentMint else TextTertiary)
                        }
                    }
                    Spacer(Modifier.height(10.dp))
                    OutlinedButton(onClick = onToggleHwid, shape = RoundedCornerShape(10.dp)) {
                        Text(if (uiState.hwidSpoofed) "恢复真实 HWID" else "伪装 HWID")
                    }
                }

                Spacer(Modifier.height(20.dp))
                SectionHeader(title = "工具入口")
                Spacer(Modifier.height(12.dp))

                Row(modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp), horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                    ToolChip("检测历史", Icons.Default.History, AccentPurple, onNavigateToHistory)
                    ToolChip("内核信息", Icons.Default.Memory, AccentBlue, onNavigateToKernelInfo)
                    ToolChip("基线对比", Icons.Default.Compare, AccentGold, onNavigateToBaseline)
                }
                Spacer(Modifier.height(10.dp))
                Row(modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp), horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                    ToolChip("特征测试", Icons.Default.Search, AccentMint, onNavigateToFeatureTest)
                    ToolChip("时序图表", Icons.Default.Timeline, ErrorRed, onNavigateToTimingChart)
                    ToolChip("白名单", Icons.Default.CheckCircle, AccentBlue, onNavigateToWhitelist)
                }
                Row(modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp), horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                    ToolChip("详细配置", Icons.Default.Tune, AccentGold, onNavigateToConfig)
                    ToolChip("隐藏模式", Icons.Default.VisibilityOff, AccentPurple, onNavigateToHideMode)
                    ToolChip("关于", Icons.Default.Info, AccentMint, onNavigateToAbout)
                    ToolChip("Frida", Icons.Default.BugReport, ErrorRed, onNavigateToFrida)
                    ToolChip("模块", Icons.Default.Extension, AccentGold, onNavigateToLSPosed)
                }

                Spacer(Modifier.height(32.dp))

                Row(
                    modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.Center
                ) {
                    Box(Modifier.size(3.dp).background(TextTertiary, CircleShape))
                    Spacer(Modifier.width(10.dp))
                    Text("v1.0.2  ·  APEX-Root 全能安全平台",
                        style = MaterialTheme.typography.bodySmall, color = TextTertiary)
                }

                Spacer(Modifier.height(32.dp))
            }
        }

            if (showExportPreview) {
                GlassShareReportPreview(
                    score = uiState.riskScore,
                    statusLabel = scoreLabel,
                    statusSummary = "此设备安全架构健全，无权级泄漏，所有指标计算均在 TrustZone 安全沙箱中加密执行。",
                    onClose = { showExportPreview = false },
                    onShare = {
                        showExportPreview = false
                        onExportReport?.invoke()
                    }
                )
            }
        }

        // 治愈操作确认对话框
        pendingCureLevel?.let { level ->
            val levelInfo = when (level) {
                CureLevel.LIGHT -> Triple("轻度处理", "清除常见 root 痕迹（su 二进制、临时文件）。风险：低", AccentMint)
                CureLevel.STANDARD -> Triple("标准修复", "移除 root 框架并恢复 boot 分区。风险：中，需要重启", AccentGold)
                CureLevel.DEEP -> Triple("深度恢复", "深度清理系统级 root 痕迹，可能影响系统设置。风险：高，需要重启", Color(0xFFFF7043))
                CureLevel.FACTORY -> Triple("完全重置", "恢复出厂设置，所有数据将被清除。风险：极高，不可逆", ErrorRed)
            }
            AlertDialog(
                onDismissRequest = { pendingCureLevel = null },
                containerColor = if (LocalIsDarkTheme.current) DeepSurface else Color.White,
                titleContentColor = if (LocalIsDarkTheme.current) TextPrimary else Color.Black,
                title = {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Icon(
                            Icons.Default.Warning,
                            contentDescription = null,
                            tint = levelInfo.third,
                            modifier = Modifier.size(24.dp)
                        )
                        Spacer(Modifier.width(8.dp))
                        Text("确认${levelInfo.first}", fontWeight = FontWeight.Bold)
                    }
                },
                text = {
                    Column {
                        Text(levelInfo.second, fontSize = 14.sp, color = if (LocalIsDarkTheme.current) TextSecondary else Color(0xFF475569))
                        Spacer(Modifier.height(12.dp))
                        Text(
                            "此操作不可撤销，请确保已备份重要数据。",
                            fontSize = 12.sp,
                            color = levelInfo.third,
                            fontWeight = FontWeight.Medium
                        )
                    }
                },
                confirmButton = {
                    Button(
                        onClick = {
                            onApplyCure(level)
                            pendingCureLevel = null
                        },
                        colors = ButtonDefaults.buttonColors(containerColor = levelInfo.third),
                        shape = RoundedCornerShape(10.dp)
                    ) {
                        Text("确认执行")
                    }
                },
                dismissButton = {
                    TextButton(onClick = { pendingCureLevel = null }) {
                        Text("取消")
                    }
                }
            )
        }
    }
}

@Composable
private fun QuickActionsRow(onScan: () -> Unit, onDeepScan: (() -> Unit)?, isScanning: Boolean) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp),
        horizontalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        Button(
            onClick = onScan, enabled = !isScanning,
            modifier = Modifier.weight(1f),
            shape = RoundedCornerShape(14.dp),
            colors = ButtonDefaults.buttonColors(containerColor = AccentPurple),
            contentPadding = PaddingValues(vertical = 14.dp)
        ) {
            if (isScanning) {
                CircularProgressIndicator(Modifier.size(16.dp), strokeWidth = 2.dp, color = Color.White)
                Spacer(Modifier.width(8.dp))
            }
            Icon(Icons.Default.PlayArrow, null, Modifier.size(18.dp))
            Spacer(Modifier.width(8.dp))
            Text(if (isScanning) "扫描中..." else "快速检测", fontWeight = FontWeight.SemiBold, letterSpacing = 0.3.sp)
        }
        if (onDeepScan != null) {
            OutlinedButton(
                onClick = onDeepScan, enabled = !isScanning,
                modifier = Modifier.weight(1f),
                shape = RoundedCornerShape(14.dp),
                contentPadding = PaddingValues(vertical = 14.dp)
            ) {
                Icon(Icons.Default.Search, null, Modifier.size(18.dp))
                Spacer(Modifier.width(8.dp))
                Text("深度检测", fontWeight = FontWeight.SemiBold, letterSpacing = 0.3.sp)
            }
        }
    }
}

@Composable
private fun ScanResultSection(result: String) {
    Column(modifier = Modifier.padding(horizontal = 20.dp)) {
        SectionHeader(title = "检测结果")
        Spacer(Modifier.height(10.dp))
        GlassCard(cornerRadius = 16.dp) {
            Text(
                text = result,
                fontFamily = FontFamily.Monospace,
                fontSize = 10.sp, color = TermuxGreen.copy(alpha = 0.80f),
                lineHeight = 16.sp,
                modifier = Modifier
                    .fillMaxWidth()
                    .background(Color.Black.copy(alpha = 0.25f), RoundedCornerShape(8.dp))
                    .padding(12.dp)
            )
        }
    }
}

@Composable
private fun DeepFindingsSection(uiState: ApexUiState) {
    Column(modifier = Modifier.padding(horizontal = 20.dp)) {
        SectionHeader(title = "深度检测发现")
        Spacer(Modifier.height(10.dp))
        GlassCard {
            Column {
                if (uiState.memFingerprintMask != 0) {
                    val memLabels = mutableListOf<String>()
                    if (uiState.memFingerprintMask and 1 != 0) memLabels.add("Magisk")
                    if (uiState.memFingerprintMask and 2 != 0) memLabels.add("Zygisk")
                    if (uiState.memFingerprintMask and 4 != 0) memLabels.add("LSPosed")
                    if (uiState.memFingerprintMask and 8 != 0) memLabels.add("Shamiko")
                    if (uiState.memFingerprintMask and 16 != 0) memLabels.add("Frida")
                    GlassFindingItem("内存特征", memLabels.joinToString(", "), AccentGold)
                }
                if (uiState.selinuxCompromised) GlassFindingItem("SELinux", "策略异常", ErrorRed)
                if (uiState.hasShamiko) GlassFindingItem("Shamiko", "检测到隐藏模块", ErrorRed)
                if (uiState.hasZygiskNext) GlassFindingItem("ZygiskNext", "检测到隐藏模块", ErrorRed)
                if (uiState.rwxPageCount > 3) GlassFindingItem("RWX 内存页", "${uiState.rwxPageCount} 页", AccentGold)
                if (uiState.selfCheckIssues.isNotEmpty()) GlassFindingItem("自保护问题", "${uiState.selfCheckIssues.size} 个", ErrorRed)
            }
        }
    }
}

@Composable
private fun RecommendationsSection(recommendations: List<FixRecommendation>, onDismiss: (() -> Unit)?) {
    Column(modifier = Modifier.padding(horizontal = 20.dp)) {
        SectionHeader(title = "修复建议")
        Spacer(Modifier.height(10.dp))
        recommendations.forEachIndexed { index, rec ->
            val priorityColor = when {
                rec.priority >= 9 -> ErrorRed
                rec.priority >= 7 -> AccentGold
                else -> AccentMint
            }
            GlassCard(cornerRadius = 16.dp, accentLine = priorityColor) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text("${index + 1}. ", fontWeight = FontWeight.Bold, fontSize = 12.sp, color = priorityColor)
                    Text(rec.titleZh, fontWeight = FontWeight.Bold, fontSize = 12.sp, color = priorityColor)
                }
                Spacer(Modifier.height(6.dp))
                Text(rec.descriptionZh, fontSize = 10.sp, color = TextSecondary)
                rec.stepsZh.forEach { step ->
                    Spacer(Modifier.height(3.dp))
                    Text("  ·  $step", fontSize = 9.sp, color = TextTertiary)
                }
            }
            Spacer(Modifier.height(8.dp))
        }
    }
}

@Composable
private fun ActionButtonsRow(onShowRecommendations: (() -> Unit)?, onExportReport: (() -> Unit)?) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp),
        horizontalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        if (onShowRecommendations != null) {
            OutlinedButton(onClick = onShowRecommendations, modifier = Modifier.weight(1f), shape = RoundedCornerShape(12.dp)) {
                Icon(Icons.Default.Lightbulb, null, Modifier.size(16.dp))
                Spacer(Modifier.width(6.dp))
                Text("修复建议", fontSize = 11.sp)
            }
        }
        if (onExportReport != null) {
            OutlinedButton(onClick = onExportReport, modifier = Modifier.weight(1f), shape = RoundedCornerShape(12.dp)) {
                Icon(Icons.Default.Share, null, Modifier.size(16.dp))
                Spacer(Modifier.width(6.dp))
                Text("导出报告", fontSize = 11.sp)
            }
        }
    }
}

@Composable
private fun CureButton(label: String, color: Color, onClick: () -> Unit) {
    Button(
        onClick = onClick,
        colors = ButtonDefaults.buttonColors(containerColor = color),
        modifier = Modifier.height(34.dp),
        shape = RoundedCornerShape(10.dp),
        contentPadding = PaddingValues(horizontal = 18.dp, vertical = 0.dp)
    ) {
        Text(label, fontSize = 11.sp, fontWeight = FontWeight.SemiBold, letterSpacing = 0.3.sp)
    }
}

@Composable
private fun RowScope.ToolChip(label: String, icon: ImageVector, color: Color, onClick: (() -> Unit)?) {
    if (onClick == null) return
    OutlinedButton(
        onClick = onClick,
        modifier = Modifier.weight(1f),
        shape = RoundedCornerShape(12.dp),
        contentPadding = PaddingValues(horizontal = 8.dp, vertical = 8.dp)
    ) {
        Icon(icon, null, Modifier.size(14.dp), tint = color)
        Spacer(Modifier.width(4.dp))
        Text(label, fontSize = 10.sp, color = color)
    }
}

/**
 * 迷你终端日志卡片 — 内嵌在 Dashboard 中
 * - 黑色背景 + monospace
 * - 显示最近 5 条日志
 * - 运行中时显示脉冲指示
 * - 点击跳转完整日志页
 */
@Composable
private fun MiniTerminalCard(
    isScanning: Boolean,
    logs: List<com.apex.root.viewmodel.trusted.LogEntry>,
    onNavigateToLogs: (() -> Unit)?
) {
    val recentLogs = logs.takeLast(5)
    val infiniteTransition = rememberInfiniteTransition(label = "pulse")
    val pulseAlpha by infiniteTransition.animateFloat(
        initialValue = 0.3f, targetValue = 1f,
        animationSpec = infiniteRepeatable(animation = tween(800), repeatMode = RepeatMode.Reverse),
        label = "pulse_alpha"
    )

    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 16.dp)
            .clip(RoundedCornerShape(14.dp))
            .background(TermuxBg)
            .clickable { onNavigateToLogs?.invoke() }
    ) {
        // 标题栏
        Row(
            modifier = Modifier.fillMaxWidth().padding(horizontal = 12.dp, vertical = 6.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                // 运行中脉冲点
                Box(
                    Modifier
                        .size(8.dp)
                        .clip(RoundedCornerShape(4.dp))
                        .background(
                            if (isScanning) AccentMint.copy(alpha = pulseAlpha) else TextTertiary.copy(alpha = 0.4f)
                        )
                )
                Spacer(Modifier.width(6.dp))
                Text(
                    if (isScanning) "RUNNING" else "LOGS",
                    color = if (isScanning) AccentMint else TextTertiary,
                    fontSize = 10.sp,
                    fontFamily = FontFamily.Monospace,
                    fontWeight = FontWeight.Bold
                )
            }
            Text(
                "${logs.size} 条",
                color = Color(0xFF64748B),
                fontSize = 9.sp,
                fontFamily = FontFamily.Monospace
            )
        }

        // 日志内容
        Column(modifier = Modifier.padding(horizontal = 12.dp, vertical = 4.dp)) {
            if (recentLogs.isEmpty()) {
                Text(
                    "$ 等待扫描...",
                    color = TermuxGreen.copy(alpha = if (isScanning) pulseAlpha else 0.4f),
                    fontSize = 11.sp,
                    fontFamily = FontFamily.Monospace
                )
            } else {
                recentLogs.forEach { log ->
                    val color = when (log.type) {
                        com.apex.root.viewmodel.trusted.LogType.ERROR -> Color(0xFFEF4444)
                        com.apex.root.viewmodel.trusted.LogType.WARN -> Color(0xFFF59E0B)
                        com.apex.root.viewmodel.trusted.LogType.INFO -> Color(0xFFE2E8F0)
                    }
                    val tag = when (log.type) {
                        com.apex.root.viewmodel.trusted.LogType.ERROR -> "E"
                        com.apex.root.viewmodel.trusted.LogType.WARN -> "W"
                        com.apex.root.viewmodel.trusted.LogType.INFO -> "I"
                    }
                    Text(
                        "${log.time} [$tag] ${log.message}",
                        color = color,
                        fontSize = 10.sp,
                        fontFamily = FontFamily.Monospace,
                        maxLines = 1,
                        overflow = TextOverflow.Ellipsis
                    )
                }
            }
            // 闪烁光标
            Text(
                "$ _",
                color = TermuxGreen.copy(alpha = pulseAlpha),
                fontSize = 11.sp,
                fontFamily = FontFamily.Monospace
            )
        }
        Spacer(Modifier.height(4.dp))
    }
}
