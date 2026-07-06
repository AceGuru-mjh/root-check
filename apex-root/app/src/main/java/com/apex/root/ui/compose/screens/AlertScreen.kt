package com.apex.root.ui.compose.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.apex.root.domain.trust.model.SecurityAlert
import com.apex.root.ui.compose.*
import com.apex.root.viewmodel.trusted.ScanViewModel
import com.apex.root.viewmodel.trusted.UiState

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun AlertScreen(
    viewModel: ScanViewModel,
    onBack: () -> Unit
) {
    val uiState by viewModel.uiState.collectAsState()
    val alerts by viewModel.alerts.collectAsState()
    val activeAlert = (uiState as? UiState.Alert)?.alert

    Scaffold(
        containerColor = Color.Transparent,
        topBar = {
            TopAppBar(
                title = { Text("安全警报", fontWeight = FontWeight.Bold, letterSpacing = 0.5.sp) },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.Default.ArrowBack, "返回")
                    }
                },
                actions = {
                    // 新增：清空所有警报按钮
                    if (alerts.isNotEmpty()) {
                        IconButton(onClick = { viewModel.clearAllAlerts() }) {
                            Icon(Icons.Default.DeleteSweep, "清空全部")
                        }
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(containerColor = Color.Transparent)
            )
        }
    ) { padding ->
        LiquidGlassContainer(fluidColorsDark = PageFluidColors.alert, fluidColorsLight = PageFluidColors.alertLight) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(padding)
                    .padding(horizontal = 16.dp, vertical = 8.dp)
            ) {
                // 当前活跃警报（如果有）— 大卡片显示
                if (activeAlert != null) {
                    ActiveAlertCard(
                        alert = activeAlert,
                        onDismiss = { viewModel.dismissAlert() }
                    )
                    Spacer(Modifier.height(14.dp))
                }

                // 警报历史列表
                if (alerts.isEmpty() && activeAlert == null) {
                    // 完全无警报
                    Box(
                        modifier = Modifier.fillMaxSize(),
                        contentAlignment = Alignment.Center
                    ) {
                        GlassEmptyState(
                            title = "无活跃警报",
                            description = "系统运行正常，未检测到任何异常指标"
                        )
                    }
                } else if (alerts.isNotEmpty()) {
                    // 历史标题
                    Row(
                        modifier = Modifier.fillMaxWidth().padding(vertical = 8.dp),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Text(
                            "警报历史 (${alerts.size})",
                            fontSize = 14.sp,
                            fontWeight = FontWeight.Bold,
                            color = TextPrimary
                        )
                    }
                    Spacer(Modifier.height(6.dp))

                    LazyColumn(
                        verticalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        itemsIndexed(alerts.reversed()) { revIdx, alert ->
                            // 原始索引（正序）
                            val origIdx = alerts.size - 1 - revIdx
                            AlertHistoryItem(
                                alert = alert,
                                onDismiss = { viewModel.dismissAlertFromHistory(origIdx) }
                            )
                        }
                    }
                }
            }
        }
    }
}

/**
 * 当前活跃警报的大卡片（带严重程度区分）。
 */
@Composable
private fun ActiveAlertCard(
    alert: SecurityAlert,
    onDismiss: () -> Unit
) {
    // 根据类型/严重程度区分颜色
    val (severityLabel, severityColor, severityIcon) = getSeverityStyle(alert)

    GlassCard(cornerRadius = 24.dp, accentLine = severityColor) {
        Column(
            modifier = Modifier.fillMaxWidth(),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Box(
                modifier = Modifier
                    .size(72.dp)
                    .clip(CircleShape)
                    .liquidGlassIcon(36.dp, severityColor.copy(alpha = 0.15f)),
                contentAlignment = Alignment.Center
            ) {
                Icon(severityIcon, null, Modifier.size(36.dp), tint = severityColor)
            }
            Spacer(Modifier.height(18.dp))
            Text(
                severityLabel,
                fontSize = 22.sp,
                fontWeight = FontWeight.Bold,
                color = severityColor,
                letterSpacing = 0.5.sp
            )
            Spacer(Modifier.height(10.dp))
            Text(
                alert.description,
                style = MaterialTheme.typography.bodyMedium,
                color = TextPrimary,
                maxLines = 6,
                overflow = androidx.compose.ui.text.style.TextOverflow.Ellipsis
            )
            Spacer(Modifier.height(24.dp))
            Box(Modifier.fillMaxWidth().height(0.5.dp)
                .background(Color.White.copy(alpha = 0.05f)))
            Spacer(Modifier.height(14.dp))
            DetailRow("类型", alert.type.name)
            DetailRow("来源", alert.sourceReplica)
            Spacer(Modifier.height(20.dp))
            Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                // 新增：确认按钮 — 清除当前活跃警报（保留历史）
                Button(
                    onClick = onDismiss,
                    shape = RoundedCornerShape(12.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = severityColor),
                    contentPadding = PaddingValues(horizontal = 28.dp, vertical = 12.dp)
                ) {
                    Icon(Icons.Default.Check, null, Modifier.size(18.dp))
                    Spacer(Modifier.width(8.dp))
                    Text("确认", fontWeight = FontWeight.SemiBold, letterSpacing = 0.3.sp)
                }
            }
        }
    }
}

/**
 * 历史警报项（紧凑卡片）。
 */
@Composable
private fun AlertHistoryItem(
    alert: SecurityAlert,
    onDismiss: () -> Unit
) {
    val (severityLabel, severityColor, severityIcon) = getSeverityStyle(alert)

    GlassCard(cornerRadius = 14.dp, accentLine = severityColor) {
        Row(
            modifier = Modifier.fillMaxWidth().padding(14.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Icon(severityIcon, null, Modifier.size(24.dp), tint = severityColor)
            Spacer(Modifier.width(12.dp))
            Column(modifier = Modifier.weight(1f)) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text(
                        severityLabel,
                        fontSize = 12.sp,
                        fontWeight = FontWeight.Bold,
                        color = severityColor
                    )
                    Text(
                        alert.type.name,
                        fontSize = 10.sp,
                        color = TextTertiary
                    )
                }
                Spacer(Modifier.height(4.dp))
                Text(
                    alert.description,
                    fontSize = 11.sp,
                    color = TextSecondary,
                    maxLines = 2,
                    overflow = androidx.compose.ui.text.style.TextOverflow.Ellipsis
                )
                Text(
                    "来源: ${alert.sourceReplica}",
                    fontSize = 9.sp,
                    color = TextTertiary
                )
            }
            Spacer(Modifier.width(8.dp))
            // 移除按钮
            IconButton(onClick = onDismiss, modifier = Modifier.size(32.dp)) {
                Icon(
                    Icons.Default.Close,
                    contentDescription = "移除",
                    tint = TextTertiary,
                    modifier = Modifier.size(16.dp)
                )
            }
        }
    }
}

/**
 * 根据警报类型返回 (标签, 颜色, 图标)。
 * 借鉴 ReportScreen 的 Severity 颜色体系。
 */
@Composable
private fun getSeverityStyle(alert: SecurityAlert): Triple<String, Color, androidx.compose.ui.graphics.vector.ImageVector> {
    val typeStr = alert.type.name.lowercase()
    return when {
        typeStr.contains("critical") || typeStr.contains("fatal") ->
            Triple("严重警报", ErrorRed, Icons.Default.Warning)
        typeStr.contains("high") || typeStr.contains("danger") ->
            Triple("高危警报", Color(0xFFFF7043), Icons.Default.Warning)
        typeStr.contains("medium") || typeStr.contains("warn") ->
            Triple("中等警报", AccentGold, Icons.Default.Notifications)
        typeStr.contains("low") || typeStr.contains("info") ->
            Triple("提示", AccentMint, Icons.Default.Info)
        else ->
            Triple("安全警报", ErrorRed, Icons.Default.Shield)
    }
}

@Composable
private fun DetailRow(label: String, value: String) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 4.dp),
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(label, color = TextTertiary, fontSize = 12.sp)
        Text(value, color = TextPrimary, fontWeight = FontWeight.Medium, fontSize = 12.sp)
    }
}
