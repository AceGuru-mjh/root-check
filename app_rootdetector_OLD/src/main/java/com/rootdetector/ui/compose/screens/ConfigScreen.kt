package com.rootdetector.ui.compose.screens

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.foundation.clickable
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
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material.icons.filled.CheckCircle
import androidx.compose.material.icons.filled.ExpandLess
import androidx.compose.material.icons.filled.ExpandMore
import androidx.compose.material.icons.filled.Security
import androidx.compose.material.icons.filled.VisibilityOff
import androidx.compose.material.icons.filled.Warning
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.FilledTonalButton
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.rootdetector.config.Category
import com.rootdetector.config.DetectionItem
import com.rootdetector.config.HideCategory
import com.rootdetector.config.HideItem
import com.rootdetector.ui.ConfigViewModel
import com.rootdetector.ui.ConfigUiState
import com.rootdetector.ui.compose.theme.AccentCyan
import com.rootdetector.ui.compose.theme.HighRed
import com.rootdetector.ui.compose.theme.MediumOrange
import com.rootdetector.ui.compose.theme.SafeGreen

@Composable
fun ConfigScreen(
    viewModel: ConfigViewModel = viewModel(),
    onBack: () -> Unit = {},
    zh: Boolean = true
) {
    val state by viewModel.state.collectAsState()

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(horizontal = 16.dp)
    ) {
        Spacer(Modifier.height(8.dp))

        Row(
            modifier = Modifier.fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically
        ) {
            IconButton(onClick = onBack) {
                Icon(Icons.Default.ArrowBack, contentDescription = null,
                    tint = MaterialTheme.colorScheme.onSurface)
            }
            Spacer(Modifier.width(8.dp))
            Column {
                Text(
                    text = if (zh) "检测与隐藏配置" else "Detection & Hide Config",
                    style = MaterialTheme.typography.headlineSmall,
                    fontWeight = FontWeight.Bold,
                    color = MaterialTheme.colorScheme.onBackground
                )
                Text(
                    text = if (zh) "自由调节检测深度与隐藏强度" else "Freely adjust detection & hide levels",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        }

        Spacer(Modifier.height(8.dp))

        TabSelector(
            activeTab = state.activeTab,
            onSelect = { viewModel.setActiveTab(it) },
            zh = zh
        )

        Spacer(Modifier.height(8.dp))

        when (state.activeTab) {
            0 -> DetectionConfigPanel(state, viewModel, zh)
            1 -> HideConfigPanel(state, viewModel, zh)
            2 -> GlobalSettingsPanel(state, viewModel, zh)
            3 -> ResultsPanel(state, viewModel, zh)
        }
    }
}

@Composable
private fun TabSelector(activeTab: Int, onSelect: (Int) -> Unit, zh: Boolean) {
    val tabs = listOf(
        if (zh) "检测配置" else "Detection",
        if (zh) "隐藏配置" else "Hiding",
        if (zh) "全局设置" else "Settings",
        if (zh) "结果" else "Results"
    )
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.spacedBy(4.dp)
    ) {
        tabs.forEachIndexed { index, label ->
            val selected = index == activeTab
            FilledTonalButton(
                onClick = { onSelect(index) },
                modifier = Modifier.weight(1f),
                shape = RoundedCornerShape(6.dp),
                colors = ButtonDefaults.filledTonalButtonColors(
                    containerColor = if (selected) AccentCyan.copy(alpha = 0.25f)
                    else MaterialTheme.colorScheme.surface
                )
            ) {
                Text(label, fontSize = 11.sp,
                    color = if (selected) AccentCyan else MaterialTheme.colorScheme.onSurface)
            }
        }
    }
}

// ==================== Detection Config Panel ====================

@Composable
private fun DetectionConfigPanel(state: ConfigUiState, vm: ConfigViewModel, zh: Boolean) {
    LazyColumn(
        modifier = Modifier.fillMaxSize(),
        verticalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        item {
            LevelSlider(
                currentLevel = state.detectionLevel,
                maxLevel = 4,
                label = if (zh) "检测深度" else "Detection Depth",
                levelLabels = if (zh) listOf("基础 L0", "标准 L1", "深度 L2", "取证 L3")
                else listOf("Basic L0", "Standard L1", "Deep L2", "Forensic L3"),
                onLevelChange = { vm.setDetectionLevel(it) },
                zh = zh
            )
        }

        item {
            Text(
                text = if (zh) "已启用: ${state.detectionEnabledItems.size}/${DetectionItem.entries.size} 项"
                else "Enabled: ${state.detectionEnabledItems.size}/${DetectionItem.entries.size} items",
                style = MaterialTheme.typography.titleSmall,
                color = AccentCyan,
                fontWeight = FontWeight.SemiBold
            )
            if (state.detectionIsCustom) {
                Text(
                    text = if (zh) "自定义模式 - 可自由开关每个检测项" else "Custom mode - toggle each item",
                    style = MaterialTheme.typography.labelSmall,
                    color = MediumOrange
                )
            }
        }

        val grouped = DetectionItem.entries.groupBy { it.category }
        grouped.forEach { (category, items) ->
            item {
                CategorySection(
                    title = if (zh) category.displayNameZh else category.displayNameEn,
                    items = items,
                    enabledItems = state.detectionCustomItems,
                    onToggle = { id, enabled -> vm.toggleDetectionItem(id, enabled) },
                    zh = zh
                )
            }
        }

        item {
            Spacer(Modifier.height(16.dp))
            Button(
                onClick = { vm.runDetection() },
                enabled = !state.isDetectionRunning,
                modifier = Modifier.fillMaxWidth(),
                shape = RoundedCornerShape(12.dp),
                colors = ButtonDefaults.buttonColors(containerColor = AccentCyan)
            ) {
                if (state.isDetectionRunning) {
                    CircularProgressIndicator(
                        modifier = Modifier.size(20.dp),
                        color = Color.White, strokeWidth = 2.dp
                    )
                    Spacer(Modifier.width(8.dp))
                    Text(if (zh) "检测中..." else "Scanning...", color = Color.White)
                } else {
                    Icon(Icons.Default.Security, contentDescription = null, tint = Color.White)
                    Spacer(Modifier.width(8.dp))
                    Text(if (zh) "开始检测 (${state.detectionEnabledItems.size}项)" else "Start Scan (${state.detectionEnabledItems.size} items)", color = Color.White)
                }
            }
            Spacer(Modifier.height(32.dp))
        }
    }
}

// ==================== Hide Config Panel ====================

@Composable
private fun HideConfigPanel(state: ConfigUiState, vm: ConfigViewModel, zh: Boolean) {
    LazyColumn(
        modifier = Modifier.fillMaxSize(),
        verticalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        item {
            LevelSlider(
                currentLevel = state.hideLevel,
                maxLevel = 4,
                label = if (zh) "隐藏强度" else "Hide Strength",
                levelLabels = if (zh) listOf("基础 H0", "标准 H1", "深度 H2", "取证 H3")
                else listOf("Basic H0", "Standard H1", "Deep H2", "Forensic H3"),
                onLevelChange = { vm.setHideLevel(it) },
                zh = zh
            )
        }

        item {
            Text(
                text = if (zh) "已启用: ${state.hideEnabledItems.size}/${HideItem.entries.size} 项"
                else "Enabled: ${state.hideEnabledItems.size}/${HideItem.entries.size} items",
                style = MaterialTheme.typography.titleSmall,
                color = AccentCyan,
                fontWeight = FontWeight.SemiBold
            )
            if (state.hideIsCustom) {
                Text(
                    text = if (zh) "自定义模式" else "Custom mode",
                    style = MaterialTheme.typography.labelSmall,
                    color = MediumOrange
                )
            }
        }

        val grouped = HideItem.entries.groupBy { it.category }
        grouped.forEach { (category, items) ->
            item {
                CategorySection(
                    title = if (zh) category.displayNameZh else category.displayNameEn,
                    items = items,
                    enabledItems = state.hideCustomItems,
                    onToggle = { id, enabled -> vm.toggleHideItem(id, enabled) },
                    zh = zh
                )
            }
        }

        item {
            Spacer(Modifier.height(8.dp))
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Button(
                    onClick = { vm.applyHide() },
                    enabled = !state.isHideRunning,
                    modifier = Modifier.weight(1f),
                    shape = RoundedCornerShape(12.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentCyan)
                ) {
                    if (state.isHideRunning) {
                        CircularProgressIndicator(modifier = Modifier.size(18.dp), color = Color.White, strokeWidth = 2.dp)
                    } else {
                        Icon(Icons.Default.VisibilityOff, contentDescription = null, tint = Color.White, modifier = Modifier.size(18.dp))
                        Spacer(Modifier.width(6.dp))
                        Text(if (zh) "应用隐藏" else "Apply Hide", color = Color.White)
                    }
                }
                OutlinedButton(
                    onClick = { vm.revertHide() },
                    enabled = !state.isHideRunning,
                    modifier = Modifier.weight(1f),
                    shape = RoundedCornerShape(12.dp)
                ) {
                    Text(if (zh) "回滚" else "Revert")
                }
            }
            Spacer(Modifier.height(32.dp))
        }
    }
}

// ==================== Global Settings Panel ====================

@Composable
private fun GlobalSettingsPanel(state: ConfigUiState, vm: ConfigViewModel, zh: Boolean) {
    LazyColumn(
        modifier = Modifier.fillMaxSize(),
        verticalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        item {
            Card(
                modifier = Modifier.fillMaxWidth(),
                shape = RoundedCornerShape(12.dp),
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surface)
            ) {
                Column(modifier = Modifier.padding(16.dp)) {
                    Text(
                        text = if (zh) "检测引擎设置" else "Detection Engine",
                        style = MaterialTheme.typography.titleMedium,
                        fontWeight = FontWeight.Bold
                    )
                    Spacer(Modifier.height(12.dp))

                    SettingToggle(
                        label = if (zh) "使用守护进程" else "Use Daemon",
                        desc = if (zh) "后台守护进程提供更深度检测" else "Background daemon for deeper detection",
                        checked = state.useDaemon,
                        onCheck = { vm.updateGlobalSetting(useDaemon = it) },
                        zh = zh
                    )
                    SettingToggle(
                        label = if (zh) "使用缓存" else "Use Cache",
                        desc = if (zh) "5分钟内相同环境复用缓存结果" else "Reuse cached results within 5 min",
                        checked = state.useCache,
                        onCheck = { vm.updateGlobalSetting(useCache = it) },
                        zh = zh
                    )
                    SettingToggle(
                        label = if (zh) "启用反反检测探针" else "Enable Anti-Hiding Probes",
                        desc = if (zh) "检测 Shamiko/ZygiskNext 等隐藏工具" else "Detect Shamiko/ZygiskNext hiding tools",
                        checked = state.enableAntiHiding,
                        onCheck = { vm.updateGlobalSetting(enableAntiHiding = it) },
                        zh = zh
                    )
                }
            }
        }

        item {
            Card(
                modifier = Modifier.fillMaxWidth(),
                shape = RoundedCornerShape(12.dp),
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surface)
            ) {
                Column(modifier = Modifier.padding(16.dp)) {
                    Text(
                        text = if (zh) "隐藏引擎设置" else "Hide Engine",
                        style = MaterialTheme.typography.titleMedium,
                        fontWeight = FontWeight.Bold
                    )
                    Spacer(Modifier.height(12.dp))

                    SettingToggle(
                        label = if (zh) "自动应用" else "Auto Apply",
                        desc = if (zh) "应用启动时自动应用隐藏策略" else "Auto-apply hide on app start",
                        checked = state.hideAutoApply,
                        onCheck = { vm.updateGlobalSetting(hideAutoApply = it) },
                        zh = zh
                    )
                    SettingToggle(
                        label = if (zh) "持久化" else "Persistent",
                        desc = if (zh) "重启后保持隐藏状态" else "Keep hide state after reboot",
                        checked = state.hidePersistent,
                        onCheck = { vm.updateGlobalSetting(hidePersistent = it) },
                        zh = zh
                    )
                }
            }
        }

        item {
            Card(
                modifier = Modifier.fillMaxWidth(),
                shape = RoundedCornerShape(12.dp),
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surface)
            ) {
                Column(modifier = Modifier.padding(16.dp)) {
                    Text(
                        text = if (zh) "关于" else "About",
                        style = MaterialTheme.typography.titleMedium,
                        fontWeight = FontWeight.Bold
                    )
                    Spacer(Modifier.height(8.dp))
                    Text(
                        text = if (zh) "检测引擎: 支持 ${DetectionItem.entries.size} 个检测项 (${Category.entries.size} 类)\n隐藏引擎: 支持 ${HideItem.entries.size} 个隐藏项 (${HideCategory.entries.size} 类)\n检测等级 L0-L3: 基础→标准→深度→取证\n隐藏等级 H0-H3: 基础→标准→深度→取证\n支持自由组合自定义模式"
                        else "Detection: ${DetectionItem.entries.size} items (${Category.entries.size} cats)\nHiding: ${HideItem.entries.size} items (${HideCategory.entries.size} cats)\nLevels L0-L3: Basic→Standard→Deep→Forensic\nLevels H0-H3: Basic→Standard→Deep→Forensic\nCustom mode: freely combine any items",
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            }
        }

        item { Spacer(Modifier.height(32.dp)) }
    }
}

// ==================== Results Panel ====================

@Composable
private fun ResultsPanel(state: ConfigUiState, vm: ConfigViewModel, zh: Boolean) {
    LazyColumn(
        modifier = Modifier.fillMaxSize(),
        verticalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        val detectionResult = state.detectionResult
        val hideReport = state.hideReport

        if (detectionResult != null) {
            item {
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    shape = RoundedCornerShape(12.dp),
                    colors = CardDefaults.cardColors(
                        containerColor = when {
                            detectionResult.threatLevel.ordinal >= 3 -> HighRed.copy(alpha = 0.15f)
                            detectionResult.threatLevel.ordinal >= 1 -> MediumOrange.copy(alpha = 0.15f)
                            else -> SafeGreen.copy(alpha = 0.15f)
                        }
                    )
                ) {
                    Column(modifier = Modifier.padding(16.dp)) {
                        Text(
                            text = if (zh) "检测结果" else "Detection Results",
                            style = MaterialTheme.typography.titleMedium,
                            fontWeight = FontWeight.Bold
                        )
                        Spacer(Modifier.height(8.dp))
                        Row(horizontalArrangement = Arrangement.spacedBy(16.dp)) {
                            StatText("${if (zh) "风险" else "Risk"}", "${detectionResult.riskScore}")
                            StatText("${if (zh) "威胁" else "Threat"}", "${detectionResult.threatLevel.name}")
                            StatText("${if (zh) "耗时" else "Time"}", "${detectionResult.elapsedMs}ms")
                            StatText("${if (zh) "启用" else "Enabled"}", "${detectionResult.enabledItems}/${detectionResult.totalItems}")
                        }
                        Spacer(Modifier.height(8.dp))
                        Text(
                            text = if (detectionResult.rootDetected) {
                                if (zh) "⚠ 检测到 Root 环境" else "⚠ Root Environment Detected"
                            } else {
                                if (zh) "✅ 设备环境安全" else "✅ Environment is Safe"
                            },
                            color = if (detectionResult.rootDetected) HighRed else SafeGreen,
                            fontWeight = FontWeight.Bold
                        )
                        Spacer(Modifier.height(8.dp))
                        if (detectionResult.layerResults.isNotEmpty()) {
                            Text(
                                text = if (zh) "检测明细 (${detectionResult.layerResults.size}项):" else "Details (${detectionResult.layerResults.size} items):",
                                style = MaterialTheme.typography.labelMedium
                            )
                            detectionResult.layerResults.forEach { r ->
                                Row(
                                    modifier = Modifier.padding(vertical = 2.dp),
                                    verticalAlignment = Alignment.CenterVertically
                                ) {
                                    Icon(
                                        imageVector = if (r.detected) Icons.Default.Warning else Icons.Default.CheckCircle,
                                        contentDescription = null,
                                        tint = if (r.detected) HighRed else SafeGreen,
                                        modifier = Modifier.size(14.dp)
                                    )
                                    Spacer(Modifier.width(4.dp))
                                    Text(
                                        text = (if (r.detail.length > 80) r.detail.take(80) + "..." else r.detail),
                                        style = MaterialTheme.typography.bodySmall,
                                        maxLines = 2, overflow = TextOverflow.Ellipsis
                                    )
                                }
                            }
                        }
                    }
                }
            }
        }

        if (hideReport != null) {
            item {
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    shape = RoundedCornerShape(12.dp),
                    colors = CardDefaults.cardColors(
                        containerColor = if (hideReport.overallSuccess) SafeGreen.copy(alpha = 0.15f)
                        else MediumOrange.copy(alpha = 0.15f)
                    )
                ) {
                    Column(modifier = Modifier.padding(16.dp)) {
                        Text(
                            text = if (zh) "隐藏结果" else "Hide Results",
                            style = MaterialTheme.typography.titleMedium,
                            fontWeight = FontWeight.Bold
                        )
                        Spacer(Modifier.height(8.dp))
                        Row(horizontalArrangement = Arrangement.spacedBy(16.dp)) {
                            StatText("${if (zh) "成功" else "OK"}", "${hideReport.successCount}")
                            StatText("${if (zh) "失败" else "Fail"}", "${hideReport.failCount}")
                        }
                        Spacer(Modifier.height(8.dp))
                        hideReport.results.forEach { r ->
                            Row(
                                modifier = Modifier.padding(vertical = 2.dp),
                                verticalAlignment = Alignment.CenterVertically
                            ) {
                                Icon(
                                    imageVector = if (r.success) Icons.Default.CheckCircle else Icons.Default.Warning,
                                    contentDescription = null,
                                    tint = if (r.success) SafeGreen else MediumOrange,
                                    modifier = Modifier.size(14.dp)
                                )
                                Spacer(Modifier.width(4.dp))
                                Text(
                                    text = "${r.itemId}: ${r.detail}",
                                    style = MaterialTheme.typography.bodySmall,
                                    maxLines = 2, overflow = TextOverflow.Ellipsis
                                )
                            }
                        }
                    }
                }
            }
        }

        if (detectionResult == null && hideReport == null) {
            item {
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(32.dp),
                    contentAlignment = Alignment.Center
                ) {
                    Text(
                        text = if (zh) "尚未运行检测或隐藏，请在对应选项卡中启动" else "No results yet. Run detection or hide from respective tabs.",
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            }
        }

        item { Spacer(Modifier.height(32.dp)) }
    }
}

// ==================== Reusable Components ====================

@Composable
private fun LevelSlider(
    currentLevel: Int,
    maxLevel: Int,
    label: String,
    levelLabels: List<String>,
    onLevelChange: (Int) -> Unit,
    zh: Boolean
) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        shape = RoundedCornerShape(12.dp),
        colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surface)
    ) {
        Column(modifier = Modifier.padding(16.dp)) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(text = label, style = MaterialTheme.typography.titleMedium, fontWeight = FontWeight.Bold)
                Text(
                    text = levelLabels.getOrElse(currentLevel - 1) { "L$currentLevel" },
                    style = MaterialTheme.typography.titleLarge,
                    fontWeight = FontWeight.ExtraBold,
                    color = AccentCyan
                )
            }
            Spacer(Modifier.height(8.dp))

            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(horizontal = 4.dp)
            ) {
                LinearProgressIndicator(
                    progress = { (currentLevel - 1).toFloat() / (maxLevel - 1).toFloat() },
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(6.dp)
                        .clip(RoundedCornerShape(3.dp)),
                    color = AccentCyan
                )
            }
            Spacer(Modifier.height(4.dp))
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                levelLabels.forEachIndexed { idx, labelText ->
                    TextButton(
                        onClick = { onLevelChange(idx + 1) },
                        contentPadding = ButtonDefaults.TextButtonContentPadding
                    ) {
                        Text(
                            text = labelText,
                            fontSize = 10.sp,
                            fontWeight = if (currentLevel == idx + 1) FontWeight.Bold else FontWeight.Normal,
                            color = if (currentLevel == idx + 1) AccentCyan else MaterialTheme.colorScheme.onSurfaceVariant
                        )
                    }
                }
            }
        }
    }
}

@Composable
private fun CategorySection(
    title: String,
    items: List<DetectionItem>,
    enabledItems: Map<String, Boolean>,
    onToggle: (String, Boolean) -> Unit,
    zh: Boolean
) {
    var expanded by remember { mutableStateOf(false) }

    Card(
        modifier = Modifier
            .fillMaxWidth()
            .clickable { expanded = !expanded },
        shape = RoundedCornerShape(8.dp),
        colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surface)
    ) {
        Column(modifier = Modifier.padding(horizontal = 12.dp, vertical = 8.dp)) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = title,
                    style = MaterialTheme.typography.labelLarge,
                    fontWeight = FontWeight.SemiBold
                )
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text(
                        text = "${items.count { enabledItems[it.id] == true }}/${items.size}",
                        style = MaterialTheme.typography.labelSmall,
                        color = AccentCyan
                    )
                    Spacer(Modifier.width(4.dp))
                    Icon(
                        imageVector = if (expanded) Icons.Default.ExpandLess else Icons.Default.ExpandMore,
                        contentDescription = null,
                        modifier = Modifier.size(18.dp),
                        tint = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            }

            AnimatedVisibility(visible = expanded) {
                Column(modifier = Modifier.padding(top = 4.dp)) {
                    items.forEach { item ->
                        val enabled = enabledItems[item.id] ?: item.defaultEnabled
                        val description = if (zh) item.descriptionZh else item.descriptionEn
                        val reference = if (item.referenceSource.isNotEmpty()) "[${item.referenceSource}] " else ""
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(vertical = 2.dp),
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.SpaceBetween
                        ) {
                            Column(modifier = Modifier.weight(1f)) {
                                Text(
                                    text = if (zh) item.displayNameZh else item.displayNameEn,
                                    style = MaterialTheme.typography.bodySmall,
                                    fontWeight = FontWeight.Medium
                                )
                                Text(
                                    text = "$reference$description",
                                    style = MaterialTheme.typography.labelSmall,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                                    maxLines = 1,
                                    overflow = TextOverflow.Ellipsis
                                )
                            }
                            Switch(
                                checked = enabled,
                                onCheckedChange = { onToggle(item.id, it) },
                                modifier = Modifier.padding(start = 8.dp)
                            )
                        }
                    }
                }
            }
        }
    }
}

private fun CategorySection(
    title: String,
    items: List<HideItem>,
    enabledItems: Map<String, Boolean>,
    onToggle: (String, Boolean) -> Unit,
    zh: Boolean
) {
    var expanded by remember { mutableStateOf(false) }

    Card(
        modifier = Modifier
            .fillMaxWidth()
            .clickable { expanded = !expanded },
        shape = RoundedCornerShape(8.dp),
        colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surface)
    ) {
        Column(modifier = Modifier.padding(horizontal = 12.dp, vertical = 8.dp)) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = title,
                    style = MaterialTheme.typography.labelLarge,
                    fontWeight = FontWeight.SemiBold
                )
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text(
                        text = "${items.count { enabledItems[it.id] == true }}/${items.size}",
                        style = MaterialTheme.typography.labelSmall,
                        color = AccentCyan
                    )
                    Spacer(Modifier.width(4.dp))
                    Icon(
                        imageVector = if (expanded) Icons.Default.ExpandLess else Icons.Default.ExpandMore,
                        contentDescription = null,
                        modifier = Modifier.size(18.dp),
                        tint = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            }

            AnimatedVisibility(visible = expanded) {
                Column(modifier = Modifier.padding(top = 4.dp)) {
                    items.forEach { item ->
                        val enabled = enabledItems[item.id] ?: item.defaultEnabled
                        val description = if (zh) item.descriptionZh else item.descriptionEn
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(vertical = 2.dp),
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.SpaceBetween
                        ) {
                            Column(modifier = Modifier.weight(1f)) {
                                Text(
                                    text = if (zh) item.displayNameZh else item.displayNameEn,
                                    style = MaterialTheme.typography.bodySmall,
                                    fontWeight = FontWeight.Medium
                                )
                                Text(
                                    text = description,
                                    style = MaterialTheme.typography.labelSmall,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                                    maxLines = 1,
                                    overflow = TextOverflow.Ellipsis
                                )
                            }
                            Switch(
                                checked = enabled,
                                onCheckedChange = { onToggle(item.id, it) },
                                modifier = Modifier.padding(start = 8.dp)
                            )
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun SettingToggle(
    label: String, desc: String, checked: Boolean,
    onCheck: (Boolean) -> Unit, zh: Boolean
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 4.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(text = label, style = MaterialTheme.typography.bodyMedium)
            Text(text = desc, style = MaterialTheme.typography.labelSmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant)
        }
        Switch(checked = checked, onCheckedChange = onCheck)
    }
}

@Composable
private fun StatText(label: String, value: String) {
    Column(horizontalAlignment = Alignment.CenterHorizontally) {
        Text(text = value, style = MaterialTheme.typography.titleSmall, fontWeight = FontWeight.Bold)
        Text(text = label, style = MaterialTheme.typography.labelSmall,
            color = MaterialTheme.colorScheme.onSurfaceVariant)
    }
}


