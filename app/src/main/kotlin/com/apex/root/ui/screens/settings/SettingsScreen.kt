package com.apex.root.ui.screens.settings

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material.icons.filled.Security
import androidx.compose.material.icons.filled.Tune
import androidx.compose.material.icons.filled.Info
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.apex.root.R
import com.apex.root.ui.components.ApexCard
import com.apex.root.ui.components.ApexTopAppBar
import com.apex.root.ui.theme.ApexRootTheme

@Composable
fun SettingsScreen(
    onNavigateBack: () -> Unit,
    modifier: Modifier = Modifier
) {
    SettingsContent(
        state = SettingsUiState(),
        onAction = {},
        onNavigateBack = onNavigateBack,
        modifier = modifier
    )
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun SettingsContent(
    state: SettingsUiState,
    onAction: (SettingsAction) -> Unit,
    onNavigateBack: () -> Unit,
    modifier: Modifier = Modifier
) {
    Scaffold(
        modifier = modifier,
        topBar = {
            ApexTopAppBar(
                title = stringResource(R.string.settings_title),
                navigationIcon = {
                    IconButton(onClick = onNavigateBack) {
                        Icon(
                            imageVector = Icons.Default.ArrowBack,
                            contentDescription = "返回"
                        )
                    }
                }
            )
        }
    ) { paddingValues ->
        LazyColumn(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues),
            contentPadding = PaddingValues(horizontal = 16.dp, vertical = 8.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            item {
                GeneralSettingsGroup(
                    autoScanEnabled = state.autoScanEnabled,
                    notificationsEnabled = state.notificationsEnabled,
                    onAutoScanChange = { onAction(SettingsAction.ToggleAutoScan(it)) },
                    onNotificationsChange = { onAction(SettingsAction.ToggleNotifications(it)) }
                )
            }
            
            item {
                DetectionLayersGroup(
                    enabledLayers = state.enabledLayers,
                    onLayerToggle = { layer, enabled -> 
                        onAction(SettingsAction.ToggleLayer(layer, enabled)) 
                    }
                )
            }
            
            item {
                AdvancedSettingsGroup(
                    sensitivityLevel = state.sensitivityLevel,
                    protectionMode = state.protectionMode,
                    onSensitivityChange = { onAction(SettingsAction.SetSensitivity(it)) },
                    onProtectionModeChange = { onAction(SettingsAction.SetProtectionMode(it)) }
                )
            }
            
            item {
                AboutSection()
            }
        }
    }
}

@Composable
private fun GeneralSettingsGroup(
    autoScanEnabled: Boolean,
    notificationsEnabled: Boolean,
    onAutoScanChange: (Boolean) -> Unit,
    onNotificationsChange: (Boolean) -> Unit,
    modifier: Modifier = Modifier
) {
    ApexCard(modifier = modifier.fillMaxWidth()) {
        Column(modifier = Modifier.padding(16.dp)) {
            Text(
                text = stringResource(R.string.settings_general),
                style = MaterialTheme.typography.titleMedium,
                color = MaterialTheme.colorScheme.primary
            )
            Spacer(modifier = Modifier.height(12.dp))
            
            SettingsSwitchItem(
                icon = Icons.Default.Security,
                title = "自动扫描",
                subtitle = "启动时自动执行检测",
                checked = autoScanEnabled,
                onCheckedChange = onAutoScanChange
            )
            
            Spacer(modifier = Modifier.height(8.dp))
            
            SettingsSwitchItem(
                icon = Icons.Default.Tune,
                title = "通知提醒",
                subtitle = "发现风险时发送通知",
                checked = notificationsEnabled,
                onCheckedChange = onNotificationsChange
            )
        }
    }
}

@Composable
private fun DetectionLayersGroup(
    enabledLayers: Set<String>,
    onLayerToggle: (String, Boolean) -> Unit,
    modifier: Modifier = Modifier
) {
    ApexCard(modifier = modifier.fillMaxWidth()) {
        Column(modifier = Modifier.padding(16.dp)) {
            Text(
                text = stringResource(R.string.settings_detection_layers),
                style = MaterialTheme.typography.titleMedium,
                color = MaterialTheme.colorScheme.primary
            )
            Spacer(modifier = Modifier.height(12.dp))
            
            val layers = listOf(
                "L1" to "系统属性检查",
                "L4" to "二进制文件检测",
                "L6" to "Native 层检测",
                "L8" to "行为分析",
                "L9" to "高级启发式",
                "L10" to "AI 检测",
                "L12" to "深度内核检查"
            )
            
            layers.forEachIndexed { index, (layerId, layerName) ->
                SettingsSwitchItem(
                    icon = Icons.Default.Security,
                    title = "$layerId - $layerName",
                    checked = enabledLayers.contains(layerId),
                    onCheckedChange = { onLayerToggle(layerId, it) }
                )
                if (index < layers.lastIndex) {
                    Spacer(modifier = Modifier.height(4.dp))
                }
            }
        }
    }
}

@Composable
private fun AdvancedSettingsGroup(
    sensitivityLevel: String,
    protectionMode: String,
    onSensitivityChange: (String) -> Unit,
    onProtectionModeChange: (String) -> Unit,
    modifier: Modifier = Modifier
) {
    ApexCard(modifier = modifier.fillMaxWidth()) {
        Column(modifier = Modifier.padding(16.dp)) {
            Text(
                text = stringResource(R.string.settings_advanced),
                style = MaterialTheme.typography.titleMedium,
                color = MaterialTheme.colorScheme.primary
            )
            Spacer(modifier = Modifier.height(12.dp))
            
            Text(
                text = "灵敏度：$sensitivityLevel",
                style = MaterialTheme.typography.bodyMedium
            )
            Spacer(modifier = Modifier.height(4.dp))
            Text(
                text = "保护模式：$protectionMode",
                style = MaterialTheme.typography.bodyMedium
            )
        }
    }
}

@Composable
private fun AboutSection(
    modifier: Modifier = Modifier
) {
    ApexCard(modifier = modifier.fillMaxWidth()) {
        Column(modifier = Modifier.padding(16.dp)) {
            Text(
                text = stringResource(R.string.settings_about),
                style = MaterialTheme.typography.titleMedium,
                color = MaterialTheme.colorScheme.primary
            )
            Spacer(modifier = Modifier.height(12.dp))
            
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Icon(
                    imageVector = Icons.Default.Info,
                    contentDescription = null,
                    tint = MaterialTheme.colorScheme.onSurfaceVariant,
                    modifier = Modifier.size(24.dp)
                )
                Spacer(modifier = Modifier.weight(1f))
                Column(horizontalAlignment = Alignment.End) {
                    Text(
                        text = "APEX Root",
                        style = MaterialTheme.typography.titleSmall
                    )
                    Text(
                        text = "版本 2.0.0-m3",
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            }
        }
    }
}

@Composable
private fun SettingsSwitchItem(
    icon: androidx.compose.ui.graphics.vector.ImageVector,
    title: String,
    subtitle: String? = null,
    checked: Boolean,
    onCheckedChange: (Boolean) -> Unit,
    modifier: Modifier = Modifier
) {
    Row(
        modifier = modifier
            .fillMaxWidth()
            .padding(vertical = 8.dp),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Row(
            modifier = Modifier.weight(1f),
            horizontalArrangement = Arrangement.spacedBy(12.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Icon(
                imageVector = icon,
                contentDescription = null,
                tint = MaterialTheme.colorScheme.onSurfaceVariant,
                modifier = Modifier.size(24.dp)
            )
            Column {
                Text(
                    text = title,
                    style = MaterialTheme.typography.bodyLarge
                )
                if (subtitle != null) {
                    Text(
                        text = subtitle,
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            }
        }
        Switch(
            checked = checked,
            onCheckedChange = onCheckedChange
        )
    }
}

data class SettingsUiState(
    val autoScanEnabled: Boolean = false,
    val notificationsEnabled: Boolean = true,
    val enabledLayers: Set<String> = setOf("L1", "L4", "L6"),
    val sensitivityLevel: String = "标准",
    val protectionMode: String = "被动"
)

sealed interface SettingsAction {
    data class ToggleAutoScan(val enabled: Boolean) : SettingsAction
    data class ToggleNotifications(val enabled: Boolean) : SettingsAction
    data class ToggleLayer(val layerId: String, val enabled: Boolean) : SettingsAction
    data class SetSensitivity(val level: String) : SettingsAction
    data class SetProtectionMode(val mode: String) : SettingsAction
}

@Preview(showBackground = true)
@Composable
private fun SettingsScreenPreview() {
    ApexRootTheme {
        SettingsScreen(onNavigateBack = {})
    }
}
