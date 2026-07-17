package com.apex.root.ui.screens.dashboard

import androidx.compose.animation.animateColorAsState
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Bolt
import androidx.compose.material.icons.filled.Search
import androidx.compose.material.icons.filled.Security
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material.icons.filled.Shield
import androidx.compose.material3.AssistChip
import androidx.compose.material3.AssistChipDefaults
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.FilledTonalButton
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.apex.root.viewmodel.trusted.ApexViewModel

/**
 * M3 Dashboard — v1.0.5
 * 完整仪表盘:风险评分环形进度条 + 扫描操作 + 20层概览 + 状态卡片
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun DashboardScreen(
    onNavigateToSettings: () -> Unit,
    onNavigateToScanResult: () -> Unit = {},
    viewModel: ApexViewModel = viewModel()
) {
    val uiState by viewModel.uiState.collectAsState()
    val scrollBehavior = TopAppBarDefaults.exitUntilCollapsedScrollBehavior()

    Scaffold(
        modifier = Modifier.nestedScroll(scrollBehavior.nestedScrollConnection),
        topBar = {
            TopAppBar(
                title = { Text("Apex Agent", fontWeight = FontWeight.Bold) },
                actions = {
                    IconButton(onClick = onNavigateToSettings) {
                        Icon(Icons.Default.Settings, contentDescription = "设置")
                    }
                },
                scrollBehavior = scrollBehavior
            )
        }
    ) { padding ->
        LazyColumn(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding),
            contentPadding = androidx.compose.foundation.layout.PaddingValues(16.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            // ═══ 风险评分环形进度条 ═══
            item { RiskScoreCard(uiState.riskScore) }

            // ═══ 扫描进度 ═══
            if (uiState.isScanning) {
                item {
                    Card(modifier = Modifier.fillMaxWidth()) {
                        Column(modifier = Modifier.padding(16.dp)) {
                            Text("扫描中", style = MaterialTheme.typography.titleSmall)
                            Spacer(Modifier.height(4.dp))
                            Text(
                                uiState.currentLayer.ifEmpty { "初始化..." },
                                style = MaterialTheme.typography.bodyMedium,
                                color = MaterialTheme.colorScheme.onSurfaceVariant
                            )
                            Spacer(Modifier.height(8.dp))
                            LinearProgressIndicator(
                                progress = { uiState.scanProgress },
                                modifier = Modifier.fillMaxWidth()
                            )
                        }
                    }
                }
            }

            // ═══ 扫描按钮组 ═══
            item {
                Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                    Button(
                        onClick = { viewModel.runScan() },
                        enabled = !uiState.isScanning,
                        modifier = Modifier.fillMaxWidth(),
                        colors = ButtonDefaults.buttonColors(
                            containerColor = MaterialTheme.colorScheme.primary
                        )
                    ) {
                        Icon(Icons.Default.Shield, contentDescription = null, modifier = Modifier.size(18.dp))
                        Spacer(Modifier.width(8.dp))
                        Text(if (uiState.isScanning) "扫描中..." else "快速检测")
                    }

                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        FilledTonalButton(
                            onClick = { viewModel.runParallelScan() },
                            enabled = !uiState.isScanning,
                            modifier = Modifier.weight(1f)
                        ) {
                            Icon(Icons.Default.Bolt, contentDescription = null, modifier = Modifier.size(16.dp))
                            Spacer(Modifier.width(4.dp))
                            Text("并行扫描", fontSize = 13.sp)
                        }

                        OutlinedButton(
                            onClick = { viewModel.runDeepScan() },
                            enabled = !uiState.isScanning,
                            modifier = Modifier.weight(1f)
                        ) {
                            Icon(Icons.Default.Search, contentDescription = null, modifier = Modifier.size(16.dp))
                            Spacer(Modifier.width(4.dp))
                            Text("深度检测", fontSize = 13.sp)
                        }
                    }
                }
            }

            // ═══ 检测层概览 ═══
            if (uiState.layersTotal > 0) {
                item {
                    Card(
                        modifier = Modifier.fillMaxWidth(),
                        onClick = onNavigateToScanResult
                    ) {
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(16.dp),
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.SpaceBetween
                        ) {
                            Column {
                                Text("检测层", style = MaterialTheme.typography.titleSmall)
                                Text(
                                    "${uiState.layersPassed}/${uiState.layersTotal} 层通过",
                                    style = MaterialTheme.typography.bodyMedium,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant
                                )
                            }
                            val passed = uiState.layersPassed.toFloat() / uiState.layersTotal.toFloat()
                            val failed = uiState.layersTotal - uiState.layersPassed
                            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                                AssistChip(
                                    onClick = {},
                                    label = { Text("✓ ${uiState.layersPassed}") },
                                    colors = AssistChipDefaults.assistChipColors(
                                        containerColor = MaterialTheme.colorScheme.secondaryContainer
                                    )
                                )
                                if (failed > 0) {
                                    AssistChip(
                                        onClick = {},
                                        label = { Text("✗ $failed") },
                                        colors = AssistChipDefaults.assistChipColors(
                                            containerColor = MaterialTheme.colorScheme.errorContainer
                                        )
                                    )
                                }
                            }
                        }
                    }
                }
            }

            // ═══ 检测结果摘要 ═══
            if (uiState.scanResult.isNotEmpty() && uiState.scanResult != "点击扫描开始检测" && !uiState.isScanning) {
                item {
                    Card(modifier = Modifier.fillMaxWidth()) {
                        Column(modifier = Modifier.padding(16.dp)) {
                            Text("检测结果", style = MaterialTheme.typography.titleSmall)
                            Spacer(Modifier.height(8.dp))
                            // 显示前 30 行
                            val lines = uiState.scanResult.lines().take(30)
                            Text(
                                lines.joinToString("\n"),
                                style = MaterialTheme.typography.bodySmall,
                                fontFamily = FontFamily.Monospace,
                                color = MaterialTheme.colorScheme.onSurfaceVariant
                            )
                            if (uiState.scanResult.lines().size > 30) {
                                Spacer(Modifier.height(4.dp))
                                Text(
                                    "... 查看完整结果",
                                    style = MaterialTheme.typography.labelSmall,
                                    color = MaterialTheme.colorScheme.primary
                                )
                            }
                        }
                    }
                }
            }

            // ═══ 深度检测发现 ═══
            if (uiState.memFingerprintMask != 0 || uiState.selinuxCompromised) {
                item {
                    Card(
                        modifier = Modifier.fillMaxWidth(),
                        colors = CardDefaults.cardColors(
                            containerColor = MaterialTheme.colorScheme.errorContainer
                        )
                    ) {
                        Column(modifier = Modifier.padding(16.dp)) {
                            Text("⚠️ 深度检测发现", style = MaterialTheme.typography.titleSmall)
                            if (uiState.memFingerprintMask != 0) {
                                Text("• 内存指纹异常 (mask=${uiState.memFingerprintMask})",
                                    style = MaterialTheme.typography.bodySmall)
                            }
                            if (uiState.selinuxCompromised) {
                                Text("• SELinux 策略可能被修改",
                                    style = MaterialTheme.typography.bodySmall)
                            }
                            if (uiState.hasShamiko) {
                                Text("• Shamiko 隐藏框架检测到",
                                    style = MaterialTheme.typography.bodySmall)
                            }
                            if (uiState.hasZygiskNext) {
                                Text("• ZygiskNext 检测到",
                                    style = MaterialTheme.typography.bodySmall)
                            }
                        }
                    }
                }
            }

            // ═══ 修复建议 ═══
            if (uiState.showRecommendations && uiState.recommendations.isNotEmpty()) {
                item {
                    Card(modifier = Modifier.fillMaxWidth()) {
                        Column(modifier = Modifier.padding(16.dp)) {
                            Text("修复建议 (${uiState.recommendations.size})",
                                style = MaterialTheme.typography.titleSmall)
                            Spacer(Modifier.height(8.dp))
                            uiState.recommendations.take(5).forEach { rec ->
                                Text(
                                    "• ${rec.titleZh}",
                                    style = MaterialTheme.typography.bodySmall,
                                    modifier = Modifier.padding(vertical = 2.dp)
                                )
                            }
                        }
                    }
                }
            }

            // ═══ 引擎状态 ═══
            item {
                Card(modifier = Modifier.fillMaxWidth()) {
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(16.dp),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Icon(
                                Icons.Default.Security,
                                contentDescription = null,
                                modifier = Modifier.size(20.dp),
                                tint = if (uiState.nativeAvailable)
                                    MaterialTheme.colorScheme.primary
                                else MaterialTheme.colorScheme.error
                            )
                            Spacer(Modifier.width(12.dp))
                            Column {
                                Text("检测引擎", style = MaterialTheme.typography.bodyMedium)
                                Text(
                                    if (uiState.nativeAvailable) "20 层 + 并行引擎" else "未加载",
                                    style = MaterialTheme.typography.bodySmall,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant
                                )
                            }
                        }
                        // 版本号
                        Text(
                            "v${com.apex.root.BuildConfig.VERSION_NAME}",
                            style = MaterialTheme.typography.labelSmall,
                            color = MaterialTheme.colorScheme.onSurfaceVariant
                        )
                    }
                }
            }

            // ═══ 实时监控快速入口 ═══
            item {
                val guardActive = uiState.guardMonitorActive
                Card(modifier = Modifier.fillMaxWidth()) {
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(16.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Box(
                            modifier = Modifier
                                .size(10.dp)
                                .padding(end = 4.dp),
                            contentAlignment = Alignment.Center
                        ) {
                            Canvas(modifier = Modifier.size(10.dp)) {
                                drawCircle(
                                    color = if (guardActive) Color(0xFF4CAF50) else Color(0xFF9E9E9E)
                                )
                            }
                        }
                        Spacer(Modifier.width(8.dp))
                        Column(modifier = Modifier.weight(1f)) {
                            Text("实时监控", style = MaterialTheme.typography.bodyMedium)
                            Text(
                                if (guardActive) "运行中 · 间隔 ${uiState.guardMonitorIntervalMs / 1000}s"
                                else "已停止",
                                style = MaterialTheme.typography.bodySmall,
                                color = MaterialTheme.colorScheme.onSurfaceVariant
                            )
                        }
                    }
                }
            }
        }
    }
}

/**
 * 环形风险评分卡片
 */
@Composable
private fun RiskScoreCard(score: Int) {
    val riskColor by animateColorAsState(
        targetValue = when {
            score > 60 -> Color(0xFFFF5252)
            score > 30 -> Color(0xFFFFC107)
            score > 10 -> Color(0xFF4CAF50)
            else -> Color(0xFF4CAF50)
        },
        label = "riskColor"
    )
    val riskLabel = when {
        score > 60 -> "高风险"
        score > 30 -> "中等风险"
        score > 10 -> "轻度风险"
        else -> "安全"
    }

    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.surfaceVariant
        )
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(24.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.SpaceEvenly
        ) {
            // 环形进度条
            Box(
                modifier = Modifier.size(100.dp),
                contentAlignment = Alignment.Center
            ) {
                Canvas(modifier = Modifier.fillMaxSize()) {
                    val strokeWidth = 8.dp.toPx()
                    val diameter = size.minDimension - strokeWidth
                    val topLeft = Offset(
                        (size.width - diameter) / 2,
                        (size.height - diameter) / 2
                    )
                    val arcSize = Size(diameter, diameter)
                    // 背景圆环
                    drawArc(
                        color = Color(0xFF2A2A35),
                        topLeft = topLeft,
                        size = arcSize,
                        startAngle = -90f,
                        sweepAngle = 360f,
                        useCenter = false,
                        style = Stroke(width = strokeWidth, cap = StrokeCap.Round)
                    )
                    // 进度圆环
                    drawArc(
                        color = riskColor,
                        topLeft = topLeft,
                        size = arcSize,
                        startAngle = -90f,
                        sweepAngle = 360f * (score / 100f),
                        useCenter = false,
                        style = Stroke(width = strokeWidth, cap = StrokeCap.Round)
                    )
                }
                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    Text(
                        "$score",
                        style = MaterialTheme.typography.headlineMedium,
                        fontWeight = FontWeight.Bold,
                        color = riskColor
                    )
                    Text(
                        "/100",
                        style = MaterialTheme.typography.labelSmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            }

            // 风险标签
            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                Text(
                    riskLabel,
                    style = MaterialTheme.typography.titleLarge,
                    fontWeight = FontWeight.Bold,
                    color = riskColor
                )
                Spacer(Modifier.height(4.dp))
                Text(
                    "风险评分",
                    style = MaterialTheme.typography.labelMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        }
    }
}
