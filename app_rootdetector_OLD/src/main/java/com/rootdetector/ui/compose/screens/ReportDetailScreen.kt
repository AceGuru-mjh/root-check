package com.rootdetector.ui.compose.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.Share
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.rootdetector.domain.model.ThreatLevel
import com.rootdetector.ui.MainViewModel
import com.rootdetector.ui.compose.components.LayerResultCard
import com.rootdetector.ui.compose.components.ThreatGauge
import com.rootdetector.ui.compose.theme.CriticalPurple
import com.rootdetector.ui.compose.theme.HighRed
import com.rootdetector.ui.compose.theme.MediumOrange
import com.rootdetector.ui.compose.theme.SafeGreen
import com.rootdetector.ui.model.DetectionUiState
import com.rootdetector.ui.model.threatLevelLabel

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ReportDetailScreen(
    viewModel: MainViewModel,
    onBack: () -> Unit = {},
    zh: Boolean = true
) {
    val uiState by viewModel.uiState.collectAsState()
    val success = uiState as? DetectionUiState.Success

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text(if (zh) "检测报告" else "Detection Report") },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = null)
                    }
                },
                actions = {
                    IconButton(onClick = { /* share */ }) {
                        Icon(Icons.Default.Share, contentDescription = null)
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = MaterialTheme.colorScheme.surface
                )
            )
        }
    ) { padding ->
        if (success == null) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(padding)
                    .padding(32.dp),
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.Center
            ) {
                Text(
                    text = if (zh) "暂无检测结果" else "No Results",
                    style = MaterialTheme.typography.titleLarge,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
            return@Scaffold
        }

        LazyColumn(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
                .padding(horizontal = 16.dp),
            verticalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            item { Spacer(modifier = Modifier.height(4.dp)) }

            item {
                SummaryHeader(success, zh)
            }

            item {
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    shape = RoundedCornerShape(12.dp),
                    colors = CardDefaults.cardColors(
                        containerColor = MaterialTheme.colorScheme.surfaceVariant.copy(alpha = 0.4f)
                    )
                ) {
                    Column(modifier = Modifier.padding(16.dp)) {
                        Text(
                            text = if (zh) "检测配置" else "Detection Info",
                            style = MaterialTheme.typography.titleMedium,
                            fontWeight = FontWeight.SemiBold
                        )
                        Spacer(modifier = Modifier.height(8.dp))
                        InfoRow(if (zh) "检测模式" else "Mode", success.level.displayName, zh)
                        InfoRow(if (zh) "耗时" else "Duration", "${success.elapsedMs}ms", zh)
                        InfoRow(if (zh) "威胁等级" else "Threat", threatLevelLabel(success.threatLevel, zh), zh)
                        InfoRow(if (zh) "异常层数" else "Detected", "${success.layerResults.count { it.detected }}/${success.layerResults.size}", zh)
                    }
                }
            }

            item {
                Text(
                    text = if (zh) "检测层详情" else "Layer Details",
                    style = MaterialTheme.typography.titleMedium,
                    fontWeight = FontWeight.SemiBold,
                    modifier = Modifier.padding(top = 8.dp, bottom = 4.dp)
                )
            }

            items(success.layerResults) { result ->
                LayerResultCard(result = result, zh = zh)
            }

            item { Spacer(modifier = Modifier.height(32.dp)) }
        }
    }
}

@Composable
private fun SummaryHeader(success: DetectionUiState.Success, zh: Boolean) {
    val gaugeColor = when (success.threatLevel) {
        ThreatLevel.SAFE -> SafeGreen
        ThreatLevel.LOW -> SafeGreen
        ThreatLevel.MEDIUM -> MediumOrange
        ThreatLevel.HIGH -> HighRed
        ThreatLevel.CRITICAL -> CriticalPurple
    }

    Card(
        modifier = Modifier.fillMaxWidth(),
        shape = RoundedCornerShape(16.dp),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.surface
        )
    ) {
        Row(
            modifier = Modifier.padding(16.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            ThreatGauge(
                riskScore = success.riskScore,
                threatLevel = success.threatLevel,
                zh = zh,
                size = 120.dp,
                strokeWidth = 10.dp
            )
            Spacer(modifier = Modifier.width(24.dp))
            Column {
                Text(
                    text = threatLevelLabel(success.threatLevel, zh),
                    style = MaterialTheme.typography.headlineMedium,
                    fontWeight = FontWeight.Bold,
                    color = gaugeColor
                )
                Spacer(modifier = Modifier.height(4.dp))
                Text(
                    text = if (success.rootDetected) {
                        if (zh) "检测到 ${success.layerResults.count { it.detected }} 项异常" else "Found ${success.layerResults.count { it.detected }} threats"
                    } else {
                        if (zh) "未发现 Root 痕迹" else "No Root traces found"
                    },
                    style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
                Spacer(modifier = Modifier.height(8.dp))
                Text(
                    text = if (zh) "风险评分: ${success.riskScore}/100" else "Risk Score: ${success.riskScore}/100",
                    style = MaterialTheme.typography.bodySmall,
                    fontWeight = FontWeight.Medium,
                    color = gaugeColor
                )
            }
        }
    }
}

@Composable
private fun InfoRow(label: String, value: String, zh: Boolean) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 2.dp),
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(
            text = label,
            style = MaterialTheme.typography.bodyMedium,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )
        Text(
            text = value,
            style = MaterialTheme.typography.bodyMedium,
            fontWeight = FontWeight.Medium,
            color = MaterialTheme.colorScheme.onSurface
        )
    }
}
