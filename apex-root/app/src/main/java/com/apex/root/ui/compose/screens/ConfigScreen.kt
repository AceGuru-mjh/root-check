package com.apex.root.ui.compose.screens

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.apex.root.ui.compose.*

data class ConfigItem(
    val id: String,
    val name: String,
    val description: String,
    val defaultEnabled: Boolean = true
)

enum class ConfigCategory(val label: String) {
    FILE_PATH("文件/路径检测"),
    MEMORY("内存检测"),
    PROCESS("进程检测"),
    SYSTEM("系统属性"),
    NETWORK("网络检测"),
    KERNEL("内核检测")
}

data class DetectionResult(
    val rootDetected: Boolean,
    val riskScore: Int,
    val threatLevel: String,
    val enabledItems: Int,
    val totalItems: Int,
    val elapsedMs: Long,
    val layerResults: List<LayerResult>
)

data class LayerResult(val detail: String, val detected: Boolean)

data class HideReport(
    val overallSuccess: Boolean,
    val successCount: Int,
    val failCount: Int,
    val results: List<HideResult>
)

data class HideResult(val itemId: String, val detail: String, val success: Boolean)

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ConfigScreen(
    onBack: () -> Unit = {},
    onRunDetection: (() -> Unit)? = null,
    onApplyHide: (() -> Unit)? = null,
    onRevertHide: (() -> Unit)? = null
) {
    var activeTab by remember { mutableIntStateOf(0) }
    val context = LocalContext.current

    val tabs = listOf("检测配置", "隐藏配置", "全局设置", "结果")
    val tabIcons = listOf(Icons.Default.Security, Icons.Default.VisibilityOff, Icons.Default.Settings, Icons.Default.Assessment)

    val scrollBehavior = TopAppBarDefaults.exitUntilCollapsedScrollBehavior()

    Scaffold(
        modifier = Modifier.nestedScroll(scrollBehavior.nestedScrollConnection),
        containerColor = Color.Transparent,
        topBar = {
            CollapsibleGlassTopBar(
                title = "检测与隐藏配置",
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
            fluidColorsDark = PageFluidColors.settings,
            fluidColorsLight = PageFluidColors.settingsLight
        ) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(padding)
                    .padding(horizontal = 16.dp)
            ) {
                Spacer(Modifier.height(8.dp))

                GlassCard(cornerRadius = 16.dp) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.spacedBy(4.dp)
                    ) {
                        tabs.forEachIndexed { index, label ->
                            val selected = index == activeTab
                            FilledTonalButton(
                                onClick = { activeTab = index },
                                modifier = Modifier.weight(1f),
                                shape = RoundedCornerShape(8.dp),
                                colors = ButtonDefaults.filledTonalButtonColors(
                                    containerColor = if (selected) AccentPurple.copy(alpha = 0.2f) else Color.Transparent
                                )
                            ) {
                                Icon(
                                    tabIcons[index],
                                    null,
                                    modifier = Modifier.size(14.dp),
                                    tint = if (selected) AccentPurple else TextTertiary
                                )
                                Spacer(Modifier.width(4.dp))
                                Text(
                                    label,
                                    fontSize = 10.sp,
                                    fontWeight = if (selected) FontWeight.SemiBold else FontWeight.Normal,
                                    color = if (selected) AccentPurple else TextTertiary
                                )
                            }
                        }
                    }
                }

                Spacer(Modifier.height(12.dp))

                when (activeTab) {
                    0 -> DetectionConfigPanel(context, onRunDetection)
                    1 -> HideConfigPanel(context, onApplyHide, onRevertHide)
                    2 -> GlobalSettingsPanel()
                    3 -> ResultsPanel()
                }
            }
        }
    }
}

// ═══════════════════════════ Detection Config ═══════════════════════════

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun DetectionConfigPanel(
    context: android.content.Context,
    onRunDetection: (() -> Unit)?
) {
    var detectionLevel by remember { mutableIntStateOf(2) }
    val levelLabels = listOf("基础 L0", "标准 L1", "深度 L2", "取证 L3")

    val detectionItems = remember {
        ConfigCategory.values().associateWith { cat ->
            when (cat) {
                ConfigCategory.FILE_PATH -> listOf(
                    ConfigItem("file_su", "su 路径检测", "检查常见 su 二进制路径"),
                    ConfigItem("file_magisk", "Magisk 路径", "检测 Magisk 安装目录"),
                    ConfigItem("file_busybox", "BusyBox 检测", "检测 BusyBox 二进制")
                )
                ConfigCategory.MEMORY -> listOf(
                    ConfigItem("mem_zygisk", "Zygisk 注入", "检测 Zygisk 内存特征"),
                    ConfigItem("mem_frida", "Frida 检测", "检测 Frida 动态插桩"),
                    ConfigItem("mem_rwx", "RWX 内存页", "检测可写可执行内存页")
                )
                ConfigCategory.PROCESS -> listOf(
                    ConfigItem("proc_magisk", "Magisk 进程", "检测 Magisk 守护进程"),
                    ConfigItem("proc_su", "su 进程检测", "检测正在运行的 su 进程")
                )
                ConfigCategory.SYSTEM -> listOf(
                    ConfigItem("sys_prop", "属性检测", "检测 ro.debuggable 等属性"),
                    ConfigItem("sys_selinux", "SELinux 状态", "检测 SELinux 是否强制模式")
                )
                ConfigCategory.NETWORK -> listOf(
                    ConfigItem("net_proxy", "代理检测", "检测系统代理设置"),
                    ConfigItem("net_vpn", "VPN 检测", "检测 VPN 连接状态")
                )
                ConfigCategory.KERNEL -> listOf(
                    ConfigItem("kern_kallsyms", "Kallsyms 检测", "检测内核符号表访问"),
                    ConfigItem("kern_syscall", "Syscall 表", "检测系统调用表篡改")
                )
            }
        }
    }

    var enabledItems by remember {
        mutableStateOf(
            detectionItems.values.flatten().associate { it.id to it.defaultEnabled }
        )
    }

    val categoryList = remember { detectionItems.entries.toList() }

    LazyColumn(
        modifier = Modifier.fillMaxSize(),
        verticalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        item {
            GlassCard(cornerRadius = 16.dp, accentLine = AccentPurple) {
                // 优化：检测深度从 FilterChip 改为 Slider 滑动开关
                Column(modifier = Modifier.fillMaxWidth()) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Column {
                            Text("检测深度", fontWeight = FontWeight.Bold, fontSize = 14.sp, color = TextPrimary)
                            Text(
                                levelLabels.getOrElse(detectionLevel - 1) { "L$detectionLevel" },
                                fontSize = 11.sp,
                                color = AccentPurple
                            )
                        }
                        Text(
                            "$detectionLevel / ${levelLabels.size}",
                            fontSize = 11.sp,
                            color = TextTertiary
                        )
                    }
                    Spacer(Modifier.height(8.dp))
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Text("浅", fontSize = 10.sp, color = TextTertiary)
                        Slider(
                            value = (detectionLevel - 1).toFloat(),
                            onValueChange = { value ->
                                detectionLevel = (value.toInt() + 1).coerceIn(1, levelLabels.size)
                            },
                            valueRange = 0f..(levelLabels.size - 1).toFloat(),
                            steps = levelLabels.size - 2,
                            modifier = Modifier.weight(1f).padding(horizontal = 8.dp),
                            colors = SliderDefaults.colors(
                                thumbColor = AccentPurple,
                                activeTrackColor = AccentPurple,
                                inactiveTrackColor = AccentPurple.copy(alpha = 0.15f)
                            )
                        )
                        Text("深", fontSize = 10.sp, color = TextTertiary)
                    }
                }
            }
        }

        item {
            Text(
                "已启用: ${enabledItems.values.count { it }}/${enabledItems.size} 项",
                fontSize = 11.sp,
                color = AccentPurple,
                fontWeight = FontWeight.SemiBold,
                modifier = Modifier.padding(horizontal = 4.dp, vertical = 4.dp)
            )
        }

        items(categoryList) { (category, items) ->
            CategoryConfigSection(
                title = category.label,
                items = items,
                enabledItems = enabledItems,
                onToggle = { id, enabled ->
                    enabledItems = enabledItems + (id to enabled)
                }
            )
        }

        item {
            Spacer(Modifier.height(8.dp))
            Button(
                onClick = {
                    // 修复：原 onClick 为空，现调用 onRunDetection 回调
                    // 若未提供回调，则使用 Toast 提示
                    if (onRunDetection != null) {
                        onRunDetection()
                    } else {
                        android.widget.Toast.makeText(context, "检测配置已保存，请前往仪表盘开始扫描", android.widget.Toast.LENGTH_SHORT).show()
                    }
                },
                modifier = Modifier.fillMaxWidth().height(48.dp),
                shape = RoundedCornerShape(14.dp),
                colors = ButtonDefaults.buttonColors(containerColor = AccentPurple)
            ) {
                Icon(Icons.Default.Security, null, Modifier.size(18.dp))
                Spacer(Modifier.width(8.dp))
                Text("开始检测 (${enabledItems.values.count { it }} 项)", fontWeight = FontWeight.SemiBold)
            }
            Spacer(Modifier.height(32.dp))
        }
    }
}

@Composable
private fun CategoryConfigSection(
    title: String,
    items: List<ConfigItem>,
    enabledItems: Map<String, Boolean>,
    onToggle: (String, Boolean) -> Unit
) {
    var expanded by remember { mutableStateOf(false) }
    val enabledCount = items.count { enabledItems[it.id] == true }

    GlassCard(
        modifier = Modifier.clickable { expanded = !expanded },
        cornerRadius = 14.dp,
        accentLine = AccentPurple
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(title, fontWeight = FontWeight.SemiBold, fontSize = 13.sp, color = TextPrimary)
                Spacer(Modifier.width(8.dp))
                Text("$enabledCount/${items.size}", fontSize = 10.sp, color = AccentPurple)
            }
            Icon(
                if (expanded) Icons.Default.ExpandLess else Icons.Default.ExpandMore,
                null, tint = TextTertiary, modifier = Modifier.size(18.dp)
            )
        }

        AnimatedVisibility(visible = expanded) {
            Column(modifier = Modifier.padding(top = 8.dp)) {
                items.forEach { item ->
                    val enabled = enabledItems[item.id] ?: item.defaultEnabled
                    Row(
                        modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Column(modifier = Modifier.weight(1f)) {
                            Text(item.name, fontSize = 12.sp, fontWeight = FontWeight.Medium, color = TextPrimary)
                            Text(item.description, fontSize = 10.sp, color = TextTertiary, maxLines = 1, overflow = TextOverflow.Ellipsis)
                        }
                        Switch(
                            checked = enabled,
                            onCheckedChange = { onToggle(item.id, it) },
                            colors = SwitchDefaults.colors(
                                checkedThumbColor = AccentPurple,
                                checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
                            )
                        )
                    }
                }
            }
        }
    }
}

// ═══════════════════════════ Hide Config ═══════════════════════════

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun HideConfigPanel(
    context: android.content.Context,
    onApplyHide: (() -> Unit)?,
    onRevertHide: (() -> Unit)?
) {
    var hideLevel by remember { mutableIntStateOf(2) }
    val levelLabels = listOf("基础 H0", "标准 H1", "深度 H2", "取证 H3")

    val hideItems = remember {
        listOf(
            ConfigItem("hide_su", "隐藏 su 文件", "屏蔽 su 二进制文件访问"),
            ConfigItem("hide_magisk", "隐藏 Magisk", "屏蔽 Magisk 相关路径"),
            ConfigItem("hide_prop", "隐藏属性", "修改 ro.debuggable 等属性"),
            ConfigItem("hide_proc", "隐藏进程", "屏蔽检测进程列表"),
            ConfigItem("hide_mount", "隐藏挂载", "屏蔽挂载点信息"),
            ConfigItem("hide_selinux", "SELinux 伪装", "伪装 SELinux 策略状态")
        )
    }

    var enabledItems by remember {
        mutableStateOf(hideItems.associate { it.id to it.defaultEnabled })
    }

    LazyColumn(
        modifier = Modifier.fillMaxSize(),
        verticalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        item {
            GlassCard(cornerRadius = 16.dp, accentLine = AccentGold) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Column {
                        Text("隐藏强度", fontWeight = FontWeight.Bold, fontSize = 14.sp, color = TextPrimary)
                        Text(levelLabels.getOrElse(hideLevel - 1) { "H$hideLevel" }, fontSize = 11.sp, color = AccentGold)
                    }
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        levelLabels.forEachIndexed { idx, label ->
                            FilterChip(
                                selected = hideLevel == idx + 1,
                                onClick = { hideLevel = idx + 1 },
                                label = { Text(label, fontSize = 8.sp) },
                                colors = FilterChipDefaults.filterChipColors(
                                    selectedContainerColor = AccentGold.copy(alpha = 0.2f),
                                    selectedLabelColor = AccentGold
                                ),
                                shape = RoundedCornerShape(6.dp),
                                modifier = Modifier.padding(horizontal = 2.dp)
                            )
                        }
                    }
                }
            }
        }

        item {
            Text(
                "已启用: ${enabledItems.values.count { it }}/${enabledItems.size} 项",
                fontSize = 11.sp, color = AccentGold, fontWeight = FontWeight.SemiBold,
                modifier = Modifier.padding(horizontal = 4.dp, vertical = 4.dp)
            )
        }

        items(hideItems) { item ->
            val enabled = enabledItems[item.id] ?: item.defaultEnabled
            GlassCard(cornerRadius = 14.dp) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Column(modifier = Modifier.weight(1f)) {
                        Text(item.name, fontSize = 13.sp, fontWeight = FontWeight.Medium, color = TextPrimary)
                        Text(item.description, fontSize = 10.sp, color = TextTertiary)
                    }
                    Switch(
                        checked = enabled,
                        onCheckedChange = { enabledItems = enabledItems + (item.id to it) },
                        colors = SwitchDefaults.colors(
                            checkedThumbColor = AccentGold,
                            checkedTrackColor = AccentGold.copy(alpha = 0.25f)
                        )
                    )
                }
            }
        }

        item {
            Spacer(Modifier.height(8.dp))
            Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                Button(
                    onClick = {
                        // 修复：原 onClick 为空，现调用 onApplyHide 回调
                        if (onApplyHide != null) {
                            onApplyHide()
                        } else {
                            android.widget.Toast.makeText(context, "隐藏配置已应用", android.widget.Toast.LENGTH_SHORT).show()
                        }
                    },
                    modifier = Modifier.weight(1f).height(44.dp),
                    shape = RoundedCornerShape(12.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentGold)
                ) {
                    Icon(Icons.Default.VisibilityOff, null, Modifier.size(16.dp))
                    Spacer(Modifier.width(6.dp))
                    Text("应用隐藏", fontWeight = FontWeight.SemiBold)
                }
                OutlinedButton(
                    onClick = {
                        // 修复：原 onClick 为空，现调用 onRevertHide 回调
                        if (onRevertHide != null) {
                            onRevertHide()
                        } else {
                            android.widget.Toast.makeText(context, "已回滚隐藏配置", android.widget.Toast.LENGTH_SHORT).show()
                        }
                    },
                    modifier = Modifier.weight(1f).height(44.dp),
                    shape = RoundedCornerShape(12.dp)
                ) {
                    Text("回滚")
                }
            }
            Spacer(Modifier.height(32.dp))
        }
    }
}

// ═══════════════════════════ Global Settings ═══════════════════════════

@Composable
private fun GlobalSettingsPanel() {
    var useDaemon by remember { mutableStateOf(true) }
    var useCache by remember { mutableStateOf(true) }
    var enableAntiHiding by remember { mutableStateOf(false) }
    var hideAutoApply by remember { mutableStateOf(false) }
    var hidePersistent by remember { mutableStateOf(true) }

    LazyColumn(
        modifier = Modifier.fillMaxSize(),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        item {
            GlassCard(cornerRadius = 16.dp, accentLine = AccentBlue) {
                SectionHeader(title = "检测引擎设置")
                Spacer(Modifier.height(12.dp))
                GlobalSettingToggle("使用守护进程", "后台守护进程提供更深度检测", useDaemon) { useDaemon = it }
                GlobalSettingToggle("使用缓存", "5 分钟内相同环境复用缓存结果", useCache) { useCache = it }
                GlobalSettingToggle("启用反反检测探针", "检测 Shamiko/ZygiskNext 等隐藏工具", enableAntiHiding) { enableAntiHiding = it }
            }
        }
        item {
            GlassCard(cornerRadius = 16.dp, accentLine = AccentGold) {
                SectionHeader(title = "隐藏引擎设置")
                Spacer(Modifier.height(12.dp))
                GlobalSettingToggle("自动应用", "应用启动时自动应用隐藏策略", hideAutoApply) { hideAutoApply = it }
                GlobalSettingToggle("持久化", "重启后保持隐藏状态", hidePersistent) { hidePersistent = it }
            }
        }
        item {
            GlassCard(cornerRadius = 16.dp, accentLine = AccentMint) {
                SectionHeader(title = "关于")
                Spacer(Modifier.height(8.dp))
                Text(
                    "检测引擎: 支持多类检测项\n隐藏引擎: 支持多级隐藏项\n检测等级 L0-L3: 基础->标准->深度->取证\n隐藏等级 H0-H3: 基础->标准->深度->取证\n支持自由组合自定义模式",
                    fontSize = 11.sp, color = TextSecondary, lineHeight = 18.sp
                )
            }
        }
        item { Spacer(Modifier.height(32.dp)) }
    }
}

@Composable
private fun GlobalSettingToggle(label: String, desc: String, checked: Boolean, onCheck: (Boolean) -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(vertical = 6.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(label, fontSize = 13.sp, fontWeight = FontWeight.Medium, color = TextPrimary)
            Text(desc, fontSize = 10.sp, color = TextTertiary)
        }
        Switch(
            checked = checked,
            onCheckedChange = onCheck,
            colors = SwitchDefaults.colors(
                checkedThumbColor = AccentPurple,
                checkedTrackColor = AccentPurple.copy(alpha = 0.25f)
            )
        )
    }
}

// ═══════════════════════════ Results ═══════════════════════════

@Composable
private fun ResultsPanel() {
    val detectionResult = remember {
        DetectionResult(
            rootDetected = true,
            riskScore = 72,
            threatLevel = "HIGH",
            enabledItems = 12,
            totalItems = 14,
            elapsedMs = 2340,
            layerResults = listOf(
                LayerResult("检查 su 路径 /system/xbin/su", true),
                LayerResult("检查 Magisk 进程", true),
                LayerResult("检查 SELinux 状态", false),
                LayerResult("检查 ro.debuggable 属性", false)
            )
        )
    }
    val hideReport = remember {
        HideReport(
            overallSuccess = true,
            successCount = 5,
            failCount = 1,
            results = listOf(
                HideResult("hide_su", "su 路径已隐藏", true),
                HideResult("hide_magisk", "Magisk 路径已屏蔽", true),
                HideResult("hide_prop", "属性已修改", true),
                HideResult("hide_proc", "进程已隐藏", true),
                HideResult("hide_mount", "挂载信息已屏蔽", true),
                HideResult("hide_selinux", "SELinux 伪装成功", false)
            )
        )
    }

    LazyColumn(
        modifier = Modifier.fillMaxSize(),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        if (detectionResult != null) {
            item {
                val bgColor = when {
                    detectionResult.threatLevel == "CRITICAL" || detectionResult.threatLevel == "HIGH" -> ErrorRed
                    detectionResult.threatLevel == "MEDIUM" -> AccentGold
                    else -> AccentMint
                }
                GlassCard(cornerRadius = 16.dp, accentLine = bgColor) {
                    SectionHeader(title = "检测结果")
                    Spacer(Modifier.height(12.dp))
                    Row(horizontalArrangement = Arrangement.spacedBy(24.dp)) {
                        StatText("风险", "${detectionResult.riskScore}")
                        StatText("威胁", detectionResult.threatLevel)
                        StatText("耗时", "${detectionResult.elapsedMs}ms")
                        StatText("启用", "${detectionResult.enabledItems}/${detectionResult.totalItems}")
                    }
                    Spacer(Modifier.height(10.dp))
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Icon(
                            if (detectionResult.rootDetected) Icons.Default.Warning else Icons.Default.CheckCircle,
                            null, tint = if (detectionResult.rootDetected) ErrorRed else AccentMint,
                            modifier = Modifier.size(18.dp)
                        )
                        Spacer(Modifier.width(8.dp))
                        Text(
                            if (detectionResult.rootDetected) "检测到 Root 环境" else "设备环境安全",
                            fontWeight = FontWeight.Bold,
                            color = if (detectionResult.rootDetected) ErrorRed else AccentMint,
                            fontSize = 14.sp
                        )
                    }
                    if (detectionResult.layerResults.isNotEmpty()) {
                        Spacer(Modifier.height(12.dp))
                        Text("检测明细 (${detectionResult.layerResults.size} 项):", fontSize = 11.sp, color = TextTertiary)
                        Spacer(Modifier.height(4.dp))
                        detectionResult.layerResults.forEach { r ->
                            Row(
                                modifier = Modifier.padding(vertical = 2.dp),
                                verticalAlignment = Alignment.CenterVertically
                            ) {
                                Icon(
                                    if (r.detected) Icons.Default.Warning else Icons.Default.CheckCircle,
                                    null, tint = if (r.detected) ErrorRed else AccentMint,
                                    modifier = Modifier.size(14.dp)
                                )
                                Spacer(Modifier.width(6.dp))
                                Text(r.detail, fontSize = 11.sp, color = TextSecondary, maxLines = 2, overflow = TextOverflow.Ellipsis)
                            }
                        }
                    }
                }
            }
        }

        if (hideReport != null) {
            item {
                val hideColor = if (hideReport.overallSuccess) AccentMint else AccentGold
                GlassCard(cornerRadius = 16.dp, accentLine = hideColor) {
                    SectionHeader(title = "隐藏结果")
                    Spacer(Modifier.height(12.dp))
                    Row(horizontalArrangement = Arrangement.spacedBy(24.dp)) {
                        StatText("成功", "${hideReport.successCount}")
                        StatText("失败", "${hideReport.failCount}")
                    }
                    Spacer(Modifier.height(8.dp))
                    hideReport.results.forEach { r ->
                        Row(
                            modifier = Modifier.padding(vertical = 2.dp),
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            Icon(
                                if (r.success) Icons.Default.CheckCircle else Icons.Default.Warning,
                                null, tint = if (r.success) AccentMint else AccentGold,
                                modifier = Modifier.size(14.dp)
                            )
                            Spacer(Modifier.width(6.dp))
                            Text("${r.itemId}: ${r.detail}", fontSize = 11.sp, color = TextSecondary, maxLines = 2, overflow = TextOverflow.Ellipsis)
                        }
                    }
                }
            }
        }

        item { Spacer(Modifier.height(32.dp)) }
    }
}

@Composable
private fun StatText(label: String, value: String) {
    Column(horizontalAlignment = Alignment.CenterHorizontally) {
        Text(value, fontSize = 16.sp, fontWeight = FontWeight.Bold, color = TextPrimary)
        Text(label, fontSize = 10.sp, color = TextTertiary)
    }
}
