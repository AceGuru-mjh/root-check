package com.apex.root.ui.screens.dashboard

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material.icons.filled.Shield
import androidx.compose.material3.AssistChip
import androidx.compose.material3.AssistChipDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.apex.root.R
import com.apex.root.domain.model.DeviceInfo
import com.apex.root.domain.model.GuardEvent
import com.apex.root.domain.model.RiskLevel
import com.apex.root.domain.model.RootDetectionResult
import com.apex.root.ui.components.ApexCard
import com.apex.root.ui.components.ApexTopAppBar
import com.apex.root.ui.theme.ApexRootTheme
import com.apex.root.ui.viewmodel.DashboardAction
import com.apex.root.ui.viewmodel.DashboardViewModel

@Composable
fun DashboardScreen(
    onNavigateToSettings: () -> Unit,
    modifier: Modifier = Modifier,
    viewModel: DashboardViewModel = viewModel()
) {
    val state by viewModel.uiState.collectAsState()
    
    DashboardContent(
        state = state,
        onAction = viewModel::onAction,
        onNavigateToSettings = onNavigateToSettings,
        modifier = modifier
    )
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun DashboardContent(
    state: DashboardUiState,
    onAction: (DashboardAction) -> Unit,
    onNavigateToSettings: () -> Unit,
    modifier: Modifier = Modifier
) {
    val scrollBehavior = TopAppBarDefaults.enterAlwaysScrollBehavior()
    
    Scaffold(
        modifier = modifier.nestedScroll(scrollBehavior.nestedScrollConnection),
        topBar = {
            ApexTopAppBar(
                title = stringResource(R.string.dashboard_title),
                subtitle = "${state.deviceInfo?.brand} ${state.deviceInfo?.model}",
                actions = {
                    IconButton(onClick = onNavigateToSettings) {
                        Icon(
                            imageVector = Icons.Default.Settings,
                            contentDescription = "设置"
                        )
                    }
                },
                scrollBehavior = scrollBehavior
            )
        }
    ) { paddingValues ->
        LazyColumn(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues),
            contentPadding = PaddingValues(horizontal = 16.dp, vertical = 8.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            item {
                DeviceSummaryCard(deviceInfo = state.deviceInfo)
            }
            
            item {
                QuickScanSection(
                    isScanning = state.isScanning,
                    progress = state.scanProgress,
                    onStartScan = { onAction(DashboardAction.StartScan) }
                )
            }
            
            if (state.lastScanResult != null) {
                item {
                    LastScanResultCard(result = state.lastScanResult)
                }
            }
            
            item {
                RiskOverviewCard(riskLevel = state.lastScanResult?.riskLevel ?: RiskLevel.NONE)
            }
            
            item {
                RecentEventsSection(events = state.recentEvents)
            }
        }
    }
}

@Composable
private fun DeviceSummaryCard(
    deviceInfo: DeviceInfo?,
    modifier: Modifier = Modifier
) {
    ApexCard(modifier = modifier.fillMaxWidth()) {
        Column(modifier = Modifier.padding(16.dp)) {
            Text(
                text = stringResource(R.string.device_summary),
                style = MaterialTheme.typography.titleMedium,
                color = MaterialTheme.colorScheme.primary
            )
            Spacer(modifier = Modifier.height(8.dp))
            if (deviceInfo != null) {
                Text(
                    text = "${deviceInfo.brand} ${deviceInfo.model}",
                    style = MaterialTheme.typography.bodyLarge
                )
                Text(
                    text = "Android ${deviceInfo.androidVersion} (API ${deviceInfo.sdkInt})",
                    style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
                Text(
                    text = "安全补丁：${deviceInfo.securityPatchLevel}",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            } else {
                Text(
                    text = stringResource(R.string.loading),
                    style = MaterialTheme.typography.bodyMedium
                )
            }
        }
    }
}

@Composable
private fun QuickScanSection(
    isScanning: Boolean,
    progress: Float,
    onStartScan: () -> Unit,
    modifier: Modifier = Modifier
) {
    ApexCard(
        modifier = modifier.fillMaxWidth(),
        onClick = if (!isScanning) onStartScan else null
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(24.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Text(
                text = if (isScanning) 
                    stringResource(R.string.scanning, (progress * 100).toInt())
                else 
                    stringResource(R.string.start_scan),
                style = MaterialTheme.typography.titleLarge,
                color = if (isScanning) 
                    MaterialTheme.colorScheme.secondary 
                else 
                    MaterialTheme.colorScheme.primary
            )
            if (isScanning) {
                Spacer(modifier = Modifier.height(16.dp))
                LinearProgressIndicator(
                    progress = progress,
                    modifier = Modifier.fillMaxWidth()
                )
            }
        }
    }
}

@Composable
private fun LastScanResultCard(
    result: RootDetectionResult,
    modifier: Modifier = Modifier
) {
    ApexCard(modifier = modifier.fillMaxWidth()) {
        Column(modifier = Modifier.padding(16.dp)) {
            Text(
                text = stringResource(R.string.last_scan),
                style = MaterialTheme.typography.titleMedium,
                color = MaterialTheme.colorScheme.primary
            )
            Spacer(modifier = Modifier.height(8.dp))
            Text(
                text = if (result.isRooted) "检测到 Root" else "未检测到 Root",
                style = MaterialTheme.typography.bodyLarge
            )
            Text(
                text = "置信度：${(result.confidence * 100).toInt()}%",
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
        }
    }
}

@Composable
private fun RiskOverviewCard(
    riskLevel: RiskLevel,
    modifier: Modifier = Modifier
) {
    ApexCard(modifier = modifier.fillMaxWidth()) {
        Column(modifier = Modifier.padding(16.dp)) {
            Text(
                text = stringResource(R.string.risk_overview),
                style = MaterialTheme.typography.titleMedium,
                color = MaterialTheme.colorScheme.primary
            )
            Spacer(modifier = Modifier.height(8.dp))
            RiskLevelBadge(riskLevel = riskLevel)
        }
    }
}

@Composable
private fun RiskLevelBadge(
    riskLevel: RiskLevel,
    modifier: Modifier = Modifier
) {
    val (label, color) = when (riskLevel) {
        RiskLevel.NONE -> stringResource(R.string.risk_none) to MaterialTheme.colorScheme.primary
        RiskLevel.LOW -> stringResource(R.string.risk_low) to MaterialTheme.colorScheme.secondary
        RiskLevel.MEDIUM -> stringResource(R.string.risk_medium) to MaterialTheme.colorScheme.tertiary
        RiskLevel.HIGH -> stringResource(R.string.risk_high) to MaterialTheme.colorScheme.error
        RiskLevel.CRITICAL -> stringResource(R.string.risk_critical) to MaterialTheme.colorScheme.onErrorContainer
    }
    
    AssistChip(
        onClick = {},
        enabled = false,
        label = { Text(label, style = MaterialTheme.typography.labelSmall) },
        leadingIcon = {
            Icon(
                imageVector = Icons.Default.Shield,
                contentDescription = null,
                tint = color,
                modifier = Modifier.size(16.dp)
            )
        },
        colors = AssistChipDefaults.assistChipColors(
            containerColor = color.copy(alpha = 0.1f)
        ),
        border = AssistChipDefaults.assistChipBorder(
            borderColor = color,
            borderWidth = 1.dp
        ),
        modifier = modifier
    )
}

@Composable
private fun RecentEventsSection(
    events: List<GuardEvent>,
    modifier: Modifier = Modifier
) {
    ApexCard(modifier = modifier.fillMaxWidth()) {
        Column(modifier = Modifier.padding(16.dp)) {
            Text(
                text = stringResource(R.string.recent_events),
                style = MaterialTheme.typography.titleMedium,
                color = MaterialTheme.colorScheme.primary
            )
            Spacer(modifier = Modifier.height(8.dp))
            if (events.isEmpty()) {
                Text(
                    text = stringResource(R.string.no_events),
                    style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        }
    }
}

@Preview(showBackground = true)
@Composable
private fun DashboardScreenPreview() {
    ApexRootTheme {
        DashboardScreen(onNavigateToSettings = {})
    }
}
