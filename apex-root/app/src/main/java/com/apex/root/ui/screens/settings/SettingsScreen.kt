package com.apex.root.ui.screens.settings

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.apex.root.viewmodel.SettingsViewModel

/**
 * M3 Settings — v1.0.4
 * 使用现有 SettingsViewModel, 精简为核心设置项。
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SettingsScreen(
    onNavigateBack: () -> Unit,
    viewModel: SettingsViewModel = viewModel()
) {
    val settings by viewModel.settings.collectAsState()
    val scrollBehavior = TopAppBarDefaults.exitUntilCollapsedScrollBehavior()

    val coreSettings = listOf(
        SettingItem("检测完成通知", "扫描完成后推送通知") { viewModel.updateNotifyScanComplete(it) },
        SettingItem("风险告警通知", "检测到高风险时推送") { viewModel.updateNotifyRiskFound(it) },
        SettingItem("防护告警通知", "Guard 实时防护告警") { viewModel.updateNotifyGuardAlert(it) },
        SettingItem("修复结果通知", "治愈操作完成后通知") { viewModel.updateNotifyCureResult(it) },
        SettingItem("更新提示通知", "有新版本时通知") { viewModel.updateNotifyUpdateAvailable(it) },
        SettingItem("自动扫描", "启动应用时自动扫描") { viewModel.updateAutoScan(it) },
        SettingItem("Guard 防护", "开启实时防护") { viewModel.updateGuardEnabled(it) }
    )

    val settingStates = listOf(
        settings.notifyScanComplete,
        settings.notifyRiskFound,
        settings.notifyGuardAlert,
        settings.notifyCureResult,
        settings.notifyUpdateAvailable,
        settings.autoScanEnabled,
        settings.guardEnabled
    )

    Scaffold(
        modifier = Modifier.nestedScroll(scrollBehavior.nestedScrollConnection),
        topBar = {
            TopAppBar(
                title = { Text("设置") },
                navigationIcon = {
                    IconButton(onClick = onNavigateBack) {
                        Icon(Icons.Default.ArrowBack, contentDescription = "返回")
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
            verticalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            item {
                Text(
                    "通知设置",
                    style = MaterialTheme.typography.titleSmall,
                    color = MaterialTheme.colorScheme.primary,
                    modifier = Modifier.padding(vertical = 8.dp)
                )
            }

            items(coreSettings.zip(settingStates)) { (item, enabled) ->
                androidx.compose.material3.Card(
                    modifier = Modifier.fillMaxWidth()
                ) {
                    androidx.compose.foundation.layout.Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(16.dp),
                        verticalAlignment = androidx.compose.ui.Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Column(modifier = Modifier.weight(1f)) {
                            Text(item.title, style = MaterialTheme.typography.bodyLarge)
                            Text(
                                item.subtitle,
                                style = MaterialTheme.typography.bodySmall,
                                color = MaterialTheme.colorScheme.onSurfaceVariant
                            )
                        }
                        Switch(
                            checked = enabled,
                            onCheckedChange = { item.onChange(it) }
                        )
                    }
                }
            }

            item {
                Text(
                    "版本: ${com.apex.root.BuildConfig.VERSION_NAME}",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                    modifier = Modifier.padding(top = 24.dp)
                )
            }
        }
    }
}

private data class SettingItem(
    val title: String,
    val subtitle: String,
    val onChange: (Boolean) -> Unit
)
