package com.rootdetector.ui.compose.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.Info
import androidx.compose.material.icons.filled.Language
import androidx.compose.material.icons.filled.Security
import androidx.compose.material.icons.filled.Storage
import androidx.compose.material.icons.filled.VerifiedUser
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Switch
import androidx.compose.material3.SwitchDefaults
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SettingsScreen(
    onBack: () -> Unit = {},
    zh: Boolean = true
) {
    var useDaemon by remember { mutableStateOf(true) }
    var cacheResults by remember { mutableStateOf(false) }
    var selfCheckOnStart by remember { mutableStateOf(true) }
    var obfuscationEnabled by remember { mutableStateOf(true) }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text(if (zh) "设置" else "Settings") },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = null)
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = MaterialTheme.colorScheme.surface
                )
            )
        }
    ) { padding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
                .padding(horizontal = 16.dp)
                .verticalScroll(rememberScrollState()),
            verticalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Spacer(modifier = Modifier.height(4.dp))

            Text(
                text = if (zh) "检测引擎" else "Detection Engine",
                style = MaterialTheme.typography.titleMedium,
                fontWeight = FontWeight.SemiBold,
                modifier = Modifier.padding(top = 8.dp)
            )

            SettingsCard {
                SettingsSwitch(
                    icon = Icons.Default.Security,
                    title = if (zh) "守护进程模式" else "Daemon Mode",
                    subtitle = if (zh) "使用独立守护进程执行检测，更安全" else "Use isolated daemon for detection",
                    checked = useDaemon,
                    onCheckedChange = { useDaemon = it },
                    zh = zh
                )
                SettingsDivider()
                SettingsSwitch(
                    icon = Icons.Default.VerifiedUser,
                    title = if (zh) "启动自检" else "Self-Check on Start",
                    subtitle = if (zh) "启动时检查应用完整性" else "Verify app integrity at startup",
                    checked = selfCheckOnStart,
                    onCheckedChange = { selfCheckOnStart = it },
                    zh = zh
                )
                SettingsDivider()
                SettingsSwitch(
                    icon = Icons.Default.Storage,
                    title = if (zh) "缓存检测结果" else "Cache Results",
                    subtitle = if (zh) "缓存上次结果，可快速查看（安全性降低）" else "Cache last result for quick view",
                    checked = cacheResults,
                    onCheckedChange = { cacheResults = it },
                    zh = zh
                )
            }

            Text(
                text = if (zh) "安全" else "Security",
                style = MaterialTheme.typography.titleMedium,
                fontWeight = FontWeight.SemiBold,
                modifier = Modifier.padding(top = 8.dp)
            )

            SettingsCard {
                SettingsSwitch(
                    icon = Icons.Default.Security,
                    title = if (zh) "混淆保护" else "Obfuscation",
                    subtitle = if (zh) "启用代码混淆和反调试机制" else "Enable code obfuscation & anti-debug",
                    checked = obfuscationEnabled,
                    onCheckedChange = { obfuscationEnabled = it },
                    zh = zh
                )
            }

            Text(
                text = if (zh) "自保护强度" else "Self-Protection Intensity",
                style = MaterialTheme.typography.titleMedium,
                fontWeight = FontWeight.SemiBold,
                modifier = Modifier.padding(top = 8.dp)
            )

            SettingsCard {
                Column(modifier = Modifier.padding(horizontal = 16.dp, vertical = 12.dp)) {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        var intensity by remember { mutableIntStateOf(1) }
                        Text(if (zh) "防护等级" else "Protection Level", modifier = Modifier.weight(1f))
                        Spacer(Modifier.width(16.dp))
                        Slider(
                            value = intensity.toFloat(),
                            onValueChange = { intensity = it.toInt() },
                            valueRange = 0f..2f,
                            steps = 1,
                            modifier = Modifier.width(180.dp)
                        )
                    }
                    Spacer(Modifier.height(4.dp))
                    Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceEvenly) {
                        Text(if (zh) "低 (性能优先)" else "Low (Performance)", style = MaterialTheme.typography.bodySmall)
                        Text(if (zh) "中 (均衡)" else "Medium (Balanced)", style = MaterialTheme.typography.bodySmall)
                        Text(if (zh) "高 (最大安全)" else "High (Max Security)", style = MaterialTheme.typography.bodySmall)
                    }
                }
            }

            Text(
                text = if (zh) "高级检测设置" else "Advanced Detection",
                style = MaterialTheme.typography.titleMedium,
                fontWeight = FontWeight.SemiBold,
                modifier = Modifier.padding(top = 8.dp)
            )

            SettingsCard {
                SettingsSwitch(
                    icon = Icons.Default.Security,
                    title = if (zh) "PMU侧信道检测" else "PMU Side-Channel",
                    subtitle = if (zh) "使用硬件性能计数器检测内核Hook" else "Use PMU for kernel hook detection",
                    checked = true,
                    onCheckedChange = {},
                    zh = zh
                )
                SettingsDivider()
                SettingsSwitch(
                    icon = Icons.Default.Security,
                    title = if (zh) "三副本交叉校验" else "Triple Arbiter",
                    subtitle = if (zh) "fork三个子进程交叉验证检测结果" else "Fork 3 children for cross-verification",
                    checked = true,
                    onCheckedChange = {},
                    zh = zh
                )
                SettingsDivider()
                SettingsSwitch(
                    icon = Icons.Default.Security,
                    title = if (zh) "AI模糊匹配" else "AI Fuzzy Matching",
                    subtitle = if (zh) "基于编辑距离的模糊特征匹配" else "Levenshtein-based fuzzy feature matching",
                    checked = true,
                    onCheckedChange = {},
                    zh = zh
                )
            }

            Text(
                text = if (zh) "关于" else "About",
                style = MaterialTheme.typography.titleMedium,
                fontWeight = FontWeight.SemiBold,
                modifier = Modifier.padding(top = 8.dp)
            )

            Card(
                modifier = Modifier.fillMaxWidth(),
                shape = RoundedCornerShape(12.dp),
                colors = CardDefaults.cardColors(
                    containerColor = MaterialTheme.colorScheme.surfaceVariant.copy(alpha = 0.4f)
                )
            ) {
                Column(modifier = Modifier.padding(16.dp)) {
                    AboutRow(
                        icon = Icons.Default.Info,
                        label = if (zh) "应用版本" else "App Version",
                        value = "1.0.0",
                        zh = zh
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                    AboutRow(
                        icon = Icons.Default.Language,
                        label = if (zh) "检测引擎" else "Detection Engine",
                        value = if (zh) "12 层插件架构" else "12-Layer Plugin Architecture",
                        zh = zh
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                    Text(
                        text = if (zh) "本应用使用隔离进程 + 加密 IPC 进行 Root 检测，支持 Magisk / KSU / APatch / LSPosed / Shamiko 等工具的检测。" else
                            "Uses isolated process + encrypted IPC for Root detection. Supports Magisk / KSU / APatch / LSPosed / Shamiko detection.",
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = 0.7f)
                    )
                }
            }

            Spacer(modifier = Modifier.height(32.dp))
        }
    }
}

@Composable
private fun SettingsCard(content: @Composable () -> Unit) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        shape = RoundedCornerShape(12.dp),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.surfaceVariant.copy(alpha = 0.4f)
        )
    ) {
        content()
    }
}

@Composable
private fun SettingsSwitch(
    icon: androidx.compose.ui.graphics.vector.ImageVector,
    title: String,
    subtitle: String,
    checked: Boolean,
    onCheckedChange: (Boolean) -> Unit,
    zh: Boolean
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 16.dp, vertical = 12.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Icon(
            imageVector = icon,
            contentDescription = null,
            tint = MaterialTheme.colorScheme.onSurfaceVariant,
            modifier = Modifier.size(24.dp)
        )
        Spacer(modifier = Modifier.width(16.dp))
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = title,
                style = MaterialTheme.typography.bodyLarge,
                color = MaterialTheme.colorScheme.onSurface
            )
            Text(
                text = subtitle,
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
        }
        Switch(
            checked = checked,
            onCheckedChange = onCheckedChange,
            colors = SwitchDefaults.colors(
                checkedTrackColor = MaterialTheme.colorScheme.primary.copy(alpha = 0.5f)
            )
        )
    }
}

@Composable
private fun SettingsDivider() {
    androidx.compose.material3.HorizontalDivider(
        modifier = Modifier.padding(horizontal = 16.dp),
        color = MaterialTheme.colorScheme.onSurfaceVariant.copy(alpha = 0.1f)
    )
}

@Composable
private fun AboutRow(
    icon: androidx.compose.ui.graphics.vector.ImageVector,
    label: String,
    value: String,
    zh: Boolean
) {
    Row(verticalAlignment = Alignment.CenterVertically) {
        Icon(
            imageVector = icon,
            contentDescription = null,
            tint = MaterialTheme.colorScheme.onSurfaceVariant,
            modifier = Modifier.size(20.dp)
        )
        Spacer(modifier = Modifier.width(12.dp))
        Text(
            text = label,
            style = MaterialTheme.typography.bodyMedium,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )
        Spacer(modifier = Modifier.weight(1f))
        Text(
            text = value,
            style = MaterialTheme.typography.bodyMedium,
            fontWeight = FontWeight.Medium,
            color = MaterialTheme.colorScheme.onSurface
        )
    }
}
