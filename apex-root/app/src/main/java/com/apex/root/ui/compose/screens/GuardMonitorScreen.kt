package com.apex.root.ui.compose.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.apex.root.domain.parallel.RealtimeGuardMonitor
import com.apex.root.ui.compose.*
import com.apex.root.viewmodel.trusted.ApexUiState
import com.apex.root.viewmodel.trusted.ApexViewModel
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * 实时监控页面 — v1.2.0 新增。
 *
 * 显示:
 *  - Guard Monitor 启停控制 + 间隔调节
 *  - 设备基线状态 (已建立/未建立/时间戳)
 *  - 最近一次检查状态 (风险分、连续异常数)
 *  - 告警历史列表 (按时间倒序,最多 50 条)
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun GuardMonitorScreen(
    uiState: ApexUiState,
    viewModel: ApexViewModel,
    onBack: () -> Unit
) {
    val scrollBehavior = TopAppBarDefaults.exitUntilCollapsedScrollBehavior()
    var intervalSliderValue by remember {
        mutableStateOf((uiState.guardMonitorIntervalMs / 1000).toFloat())
    }

    Scaffold(
        modifier = Modifier.nestedScroll(scrollBehavior.nestedScrollConnection),
        containerColor = Color.Transparent,
        topBar = {
            CollapsibleGlassTopBar(
                title = "实时监控",
                collapsedFraction = scrollBehavior.state.collapsedFraction,
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.Default.ArrowBack, "返回")
                    }
                }
            )
        }
    ) { padding ->
        LiquidGlassContainer(
            fluidColorsDark = PageFluidColors.dashboard,
            fluidColorsLight = PageFluidColors.dashboardLight
        ) {
            LazyColumn(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(padding)
                    .padding(16.dp),
                verticalArrangement = Arrangement.spacedBy(12.dp),
                contentPadding = PaddingValues(vertical = 8.dp)
            ) {
                // ─── 监控状态卡片 ───
                item { SectionHeader(title = "监控状态") }
                item {
                    GlassCard(cornerRadius = 16.dp) {
                        Column(modifier = Modifier.padding(16.dp)) {
                            Row(
                                modifier = Modifier.fillMaxWidth(),
                                horizontalArrangement = Arrangement.SpaceBetween,
                                verticalAlignment = Alignment.CenterVertically
                            ) {
                                Column {
                                    Text(
                                        text = if (uiState.guardMonitorActive) "🟢 监控中" else "⚪ 已停止",
                                        fontSize = 18.sp,
                                        fontWeight = FontWeight.Bold,
                                        color = if (uiState.guardMonitorActive) AccentMint else TextSecondary
                                    )
                                    Text(
                                        text = "间隔: ${uiState.guardMonitorIntervalMs / 1000}s",
                                        fontSize = 12.sp,
                                        color = TextSecondary
                                    )
                                }
                                Switch(
                                    checked = uiState.guardMonitorActive,
                                    onCheckedChange = { enabled ->
                                        if (enabled) {
                                            viewModel.startGuardMonitor((intervalSliderValue * 1000).toLong())
                                        } else {
                                            viewModel.stopGuardMonitor()
                                        }
                                    }
                                )
                            }
                            Spacer(Modifier.height(12.dp))
                            // 间隔调节滑块
                            Text("检测间隔: ${intervalSliderValue.toInt()}秒", fontSize = 13.sp, color = TextPrimary)
                            Slider(
                                value = intervalSliderValue,
                                onValueChange = { intervalSliderValue = it },
                                onValueChangeFinished = {
                                    viewModel.updateGuardInterval((intervalSliderValue * 1000).toLong())
                                },
                                valueRange = 30f..600f,
                                steps = 18,  // 30s 步进
                                enabled = uiState.guardMonitorActive
                            )
                            Row(
                                modifier = Modifier.fillMaxWidth(),
                                horizontalArrangement = Arrangement.SpaceBetween
                            ) {
                                Text("30s", fontSize = 10.sp, color = TextSecondary)
                                Text("600s", fontSize = 10.sp, color = TextSecondary)
                            }
                            Spacer(Modifier.height(8.dp))
                            Row(
                                modifier = Modifier.fillMaxWidth(),
                                horizontalArrangement = Arrangement.spacedBy(8.dp)
                            ) {
                                OutlinedButton(
                                    onClick = { viewModel.checkGuardNow() },
                                    enabled = uiState.guardMonitorActive,
                                    modifier = Modifier.weight(1f)
                            ) {
                                    Icon(Icons.Default.Refresh, null, Modifier.size(16.dp))
                                    Spacer(Modifier.width(4.dp))
                                    Text("立即检查", fontSize = 12.sp)
                                }
                            }
                        }
                    }
                }

                // ─── 基线状态卡片 ───
                item { SectionHeader(title = "设备基线") }
                item {
                    GlassCard(cornerRadius = 16.dp) {
                        Column(modifier = Modifier.padding(16.dp)) {
                            Row(
                                modifier = Modifier.fillMaxWidth(),
                                horizontalArrangement = Arrangement.SpaceBetween,
                                verticalAlignment = Alignment.CenterVertically
                            ) {
                                Column {
                                    Text(
                                        text = if (uiState.hasBaseline) "✅ 基线已建立" else "❌ 未建立基线",
                                        fontSize = 14.sp,
                                        fontWeight = FontWeight.SemiBold,
                                        color = if (uiState.hasBaseline) AccentMint else ErrorRed
                                    )
                                    uiState.baselineTimestamp?.let { ts ->
                                        Text(
                                            text = "建立于: ${formatTimestamp(ts)}",
                                            fontSize = 11.sp,
                                            color = TextSecondary
                                        )
                                    }
                                }
                                Row(horizontalArrangement = Arrangement.spacedBy(4.dp)) {
                                    OutlinedButton(onClick = { viewModel.establishBaseline() }) {
                                        Text("重建", fontSize = 11.sp)
                                    }
                                    if (uiState.hasBaseline) {
                                        OutlinedButton(onClick = { viewModel.clearBaseline() }) {
                                            Text("清除", fontSize = 11.sp)
                                        }
                                    }
                                }
                            }
                            Spacer(Modifier.height(8.dp))
                            Text(
                                text = "基线用于检测设备状态变化。首次扫描时自动建立,后续扫描与基线对比以发现新增 root 痕迹。",
                                fontSize = 11.sp,
                                color = TextSecondary,
                                lineHeight = 16.sp
                            )
                        }
                    }
                }

                // ─── 最近检查状态 ───
                if (uiState.lastGuardCheckTimestamp != null) {
                    item { SectionHeader(title = "最近检查") }
                    item {
                        GlassCard(cornerRadius = 16.dp) {
                            Column(modifier = Modifier.padding(16.dp)) {
                                Row(modifier = Modifier.fillMaxWidth()) {
                                    InfoChip("最后检查", formatTimestamp(uiState.lastGuardCheckTimestamp!!))
                                    Spacer(Modifier.width(8.dp))
                                    InfoChip("连续异常", "${uiState.consecutiveAnomalies}")
                                }
                            }
                        }
                    }
                }

                // ─── 告警历史 ───
                item { SectionHeader(title = "告警历史 (${uiState.guardAlerts.size})") }
                if (uiState.guardAlerts.isEmpty()) {
                    item {
                        GlassCard(cornerRadius = 16.dp) {
                            Box(
                                modifier = Modifier.fillMaxWidth().padding(32.dp),
                                contentAlignment = Alignment.Center
                            ) {
                                Text("暂无告警", color = TextSecondary, fontSize = 13.sp)
                            }
                        }
                    }
                } else {
                    item {
                        Row(
                            modifier = Modifier.fillMaxWidth(),
                            horizontalArrangement = Arrangement.End
                        ) {
                            TextButton(onClick = { viewModel.clearGuardAlerts() }) {
                                Text("清空历史", fontSize = 11.sp, color = TextSecondary)
                            }
                        }
                    }
                    items(uiState.guardAlerts.reversed()) { alert ->
                        GuardAlertCard(alert)
                    }
                }

                item { Spacer(Modifier.height(32.dp)) }
            }
        }
    }
}

@Composable
private fun GuardAlertCard(alert: RealtimeGuardMonitor.GuardAlert) {
    val (bgColor, iconColor, icon) = when (alert.level) {
        RealtimeGuardMonitor.AlertLevel.CRITICAL -> Triple(
            Color(0x33FF4444), ErrorRed, Icons.Default.Warning
        )
        RealtimeGuardMonitor.AlertLevel.ALERT -> Triple(
            Color(0x33FF8800), Color(0xFFFF8800), Icons.Default.Warning
        )
        RealtimeGuardMonitor.AlertLevel.WARNING -> Triple(
            Color(0x33FFC107), Color(0xFFFFC107), Icons.Default.Info
        )
        RealtimeGuardMonitor.AlertLevel.INFO -> Triple(
            Color(0x3300BCD4), AccentCyan, Icons.Default.Info
        )
    }
    GlassCard(cornerRadius = 12.dp) {
        Column(modifier = Modifier.padding(12.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Box(
                    modifier = Modifier.size(28.dp).background(bgColor, RoundedCornerShape(14.dp)),
                    contentAlignment = Alignment.Center
                ) {
                    Icon(icon, null, tint = iconColor, modifier = Modifier.size(16.dp))
                }
                Spacer(Modifier.width(8.dp))
                Column(modifier = Modifier.weight(1f)) {
                    Text(alert.title, fontSize = 13.sp, fontWeight = FontWeight.SemiBold, color = TextPrimary)
                    Text(formatTimestamp(alert.timestamp), fontSize = 10.sp, color = TextSecondary)
                }
            }
            Spacer(Modifier.height(6.dp))
            Text(
                text = alert.message,
                fontSize = 11.sp,
                color = TextSecondary,
                lineHeight = 15.sp,
                fontFamily = FontFamily.Monospace
            )
        }
    }
}

@Composable
private fun InfoChip(label: String, value: String) {
    Column {
        Text(label, fontSize = 10.sp, color = TextSecondary)
        Text(value, fontSize = 12.sp, color = TextPrimary, fontWeight = FontWeight.Medium)
    }
}

private fun formatTimestamp(ts: Long): String {
    return SimpleDateFormat("MM-dd HH:mm:ss", Locale.getDefault()).format(Date(ts))
}
