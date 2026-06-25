package com.rootdetector.ui.compose.screens

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.foundation.background
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
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.History
import androidx.compose.material.icons.filled.Info
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material.icons.filled.Security
import androidx.compose.material.icons.filled.Speed
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.FilledTonalButton
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.rootdetector.domain.model.DetectionLevel
import com.rootdetector.ui.MainViewModel
import com.rootdetector.ui.compose.components.LayerResultCard
import com.rootdetector.ui.compose.components.ThreatGauge
import com.rootdetector.ui.compose.theme.AccentCyan
import com.rootdetector.ui.compose.theme.CriticalPurple
import com.rootdetector.config.DetectionItem
import com.rootdetector.ui.compose.theme.HighRed
import com.rootdetector.ui.compose.theme.MediumOrange
import com.rootdetector.ui.compose.theme.SafeGreen
import com.rootdetector.ui.model.DetectionUiState

@Composable
fun DashboardScreen(
    viewModel: MainViewModel = viewModel(),
    onNavigateToReport: () -> Unit = {},
    onNavigateToHistory: () -> Unit = {},
    onNavigateToSettings: () -> Unit = {},
    onNavigateToKernelInfo: () -> Unit = {},
    onNavigateToBaseline: () -> Unit = {},
    onNavigateToFeatureTest: () -> Unit = {},
    onNavigateToTimingChart: () -> Unit = {},
    onNavigateToWhitelist: () -> Unit = {},
    onNavigateToConfig: () -> Unit = {},
    zh: Boolean = true
) {
    val uiState by viewModel.uiState.collectAsState()

    LazyColumn(
        modifier = Modifier
            .fillMaxSize()
            .padding(horizontal = 16.dp),
        verticalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        item {
            Spacer(modifier = Modifier.height(16.dp))
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Column {
                    Text(
                        text = if (zh) "Root 环境检测" else "Root Environment",
                        style = MaterialTheme.typography.headlineLarge,
                        fontWeight = FontWeight.Bold,
                        color = MaterialTheme.colorScheme.onBackground
                    )
                    Text(
                        text = if (zh) "17 层深度检测引擎" else "17-Layer Detection Engine",
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
                Row {
                    IconButton(onClick = onNavigateToConfig) {
                        Icon(Icons.Default.Security, contentDescription = null, tint = AccentCyan)
                    }
                    IconButton(onClick = onNavigateToHistory) {
                        Icon(Icons.Default.History, contentDescription = null, tint = MaterialTheme.colorScheme.onSurfaceVariant)
                    }
                    IconButton(onClick = onNavigateToSettings) {
                        Icon(Icons.Default.Settings, contentDescription = null, tint = MaterialTheme.colorScheme.onSurfaceVariant)
                    }
                }
            }
        }

        item {
            Spacer(modifier = Modifier.height(8.dp))
            ModeSelector(
                currentLevel = (uiState as? DetectionUiState.Success)?.level,
                onSelect = { viewModel.runDetection(it) },
                enabled = uiState !is DetectionUiState.Running,
                zh = zh
            )
        }

        item {
            Spacer(Modifier.height(8.dp))
            Text(
                text = if (zh) "诊断工具" else "Diagnostic Tools",
                style = MaterialTheme.typography.titleSmall,
                fontWeight = FontWeight.SemiBold,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
            Spacer(Modifier.height(8.dp))
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                FilledTonalButton(
                    onClick = onNavigateToKernelInfo,
                    modifier = Modifier.weight(1f)
                ) {
                    Icon(Icons.Default.Info, contentDescription = null, modifier = Modifier.size(16.dp))
                    Spacer(Modifier.width(4.dp))
                    Text(if (zh) "内核" else "Kernel", fontSize = 11.sp)
                }
                FilledTonalButton(
                    onClick = onNavigateToBaseline,
                    modifier = Modifier.weight(1f)
                ) {
                    Icon(Icons.Default.Speed, contentDescription = null, modifier = Modifier.size(16.dp))
                    Spacer(Modifier.width(4.dp))
                    Text(if (zh) "基线" else "Baseline", fontSize = 11.sp)
                }
                FilledTonalButton(
                    onClick = onNavigateToFeatureTest,
                    modifier = Modifier.weight(1f)
                ) {
                    Icon(Icons.Default.PlayArrow, contentDescription = null, modifier = Modifier.size(16.dp))
                    Spacer(Modifier.width(4.dp))
                    Text(if (zh) "测试" else "Test", fontSize = 11.sp)
                }
            }
            Spacer(Modifier.height(4.dp))
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                FilledTonalButton(
                    onClick = onNavigateToTimingChart,
                    modifier = Modifier.weight(1f)
                ) {
                    Icon(Icons.Default.Speed, contentDescription = null, modifier = Modifier.size(16.dp))
                    Spacer(Modifier.width(4.dp))
                    Text(if (zh) "时延图" else "Timing", fontSize = 11.sp)
                }
                FilledTonalButton(
                    onClick = onNavigateToWhitelist,
                    modifier = Modifier.weight(1f)
                ) {
                    Icon(Icons.Default.Info, contentDescription = null, modifier = Modifier.size(16.dp))
                    Spacer(Modifier.width(4.dp))
                    Text(if (zh) "白名单" else "Whitelist", fontSize = 11.sp)
                }
            }
        }

        item {
            AnimatedVisibility(
                visible = uiState is DetectionUiState.Running,
                enter = fadeIn(),
                exit = fadeOut()
            ) {
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(32.dp),
                    contentAlignment = Alignment.Center
                ) {
                    Column(horizontalAlignment = Alignment.CenterHorizontally) {
                        CircularProgressIndicator(
                            modifier = Modifier.size(64.dp),
                            color = AccentCyan,
                            strokeWidth = 5.dp
                        )
                        Spacer(modifier = Modifier.height(16.dp))
                        Text(
                            text = if (zh) "正在检测..." else "Scanning...",
                            style = MaterialTheme.typography.titleMedium,
                            color = MaterialTheme.colorScheme.onSurfaceVariant
                        )
                    }
                }
            }
        }

        if (uiState is DetectionUiState.Success) {
            val success = uiState as DetectionUiState.Success
            item {
                ResultSummaryCard(success, zh)
            }

            item {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    FilledTonalButton(
                        onClick = onNavigateToReport,
                        modifier = Modifier.weight(1f)
                    ) {
                        Icon(Icons.Default.Info, contentDescription = null, modifier = Modifier.size(18.dp))
                        Spacer(modifier = Modifier.width(6.dp))
                        Text(if (zh) "查看报告" else "Report")
                    }
                    FilledTonalButton(
                        onClick = { viewModel.resetState() },
                        modifier = Modifier.weight(1f)
                    ) {
                        Icon(Icons.Default.PlayArrow, contentDescription = null, modifier = Modifier.size(18.dp))
                        Spacer(modifier = Modifier.width(6.dp))
                        Text(if (zh) "重新检测" else "Retry")
                    }
                }
            }

            items(success.layerResults) { result ->
                LayerResultCard(result = result, zh = zh)
            }
        }

        item {
            Spacer(modifier = Modifier.height(32.dp))
        }
    }
}

@Composable
private fun ModeSelector(
    currentLevel: DetectionLevel?,
    onSelect: (DetectionLevel) -> Unit,
    enabled: Boolean,
    zh: Boolean
) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        DetectionLevel.entries.forEach { level ->
            val selected = currentLevel == level
            Button(
                onClick = { onSelect(level) },
                enabled = enabled,
                modifier = Modifier.weight(1f),
                shape = RoundedCornerShape(8.dp),
                colors = ButtonDefaults.buttonColors(
                    containerColor = if (selected) AccentCyan.copy(alpha = 0.3f)
                    else MaterialTheme.colorScheme.surface,
                    contentColor = if (selected) AccentCyan
                    else MaterialTheme.colorScheme.onSurface
                ),
                elevation = ButtonDefaults.buttonElevation(defaultElevation = 0.dp)
            ) {
                Text(
                    text = level.displayName,
                    fontSize = 12.sp,
                    fontWeight = if (selected) FontWeight.Bold else FontWeight.Normal,
                    maxLines = 1
                )
            }
        }
    }
}

@Composable
private fun ResultSummaryCard(success: DetectionUiState.Success, zh: Boolean) {
    val threatColor = when (success.threatLevel) {
        com.rootdetector.domain.model.ThreatLevel.SAFE -> SafeGreen
        com.rootdetector.domain.model.ThreatLevel.LOW -> Color(0xFFFFC107)
        com.rootdetector.domain.model.ThreatLevel.MEDIUM -> MediumOrange
        com.rootdetector.domain.model.ThreatLevel.HIGH -> HighRed
        com.rootdetector.domain.model.ThreatLevel.CRITICAL -> CriticalPurple
    }

    Card(
        modifier = Modifier.fillMaxWidth(),
        shape = RoundedCornerShape(16.dp),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.surface.copy(alpha = 0.7f)
        )
    ) {
        Column(
            modifier = Modifier.padding(16.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            ThreatGauge(
                riskScore = success.riskScore,
                threatLevel = success.threatLevel,
                zh = zh,
                size = 180.dp
            )

            Spacer(modifier = Modifier.height(12.dp))

            Text(
                text = if (success.rootDetected) {
                    if (zh) "检测到 Root 环境" else "Root Environment Detected"
                } else {
                    if (zh) "设备环境安全" else "Environment is Safe"
                },
                style = MaterialTheme.typography.titleLarge,
                fontWeight = FontWeight.Bold,
                color = threatColor,
                textAlign = TextAlign.Center
            )

            Spacer(modifier = Modifier.height(8.dp))

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceEvenly
            ) {
                StatItem(
                    value = "${success.layerResults.size}",
                    label = if (zh) "检测项" else "Checks"
                )
                StatItem(
                    value = "${success.layerResults.count { it.detected }}",
                    label = if (zh) "异常" else "Threats"
                )
                StatItem(
                    value = "${success.elapsedMs}ms",
                    label = if (zh) "耗时" else "Duration"
                )
                StatItem(
                    value = "${success.riskScore}",
                    label = if (zh) "风险分" else "Risk"
                )
            }
        }
    }
}

@Composable
private fun StatItem(value: String, label: String) {
    Column(horizontalAlignment = Alignment.CenterHorizontally) {
        Text(
            text = value,
            style = MaterialTheme.typography.titleMedium,
            fontWeight = FontWeight.Bold,
            color = MaterialTheme.colorScheme.onSurface
        )
        Text(
            text = label,
            style = MaterialTheme.typography.labelSmall,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )
    }
}
