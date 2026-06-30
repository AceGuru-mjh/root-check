package com.apex.root.ui.compose.screens

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.background
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.apex.root.island.HideModeManager
import com.apex.root.ui.compose.AccentPurple
import com.apex.root.ui.compose.DeepSurface
import com.apex.root.ui.compose.LightSurface
import com.apex.root.ui.compose.TextPrimary
import com.apex.root.ui.compose.TextSecondary
import com.apex.root.ui.compose.TextTertiary
import com.apex.root.ui.compose.liquidGlass
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * 隐藏模式控制屏幕
 *
 * 提供 Detection / Hide / Game 三种模式的一键切换 UI。
 * 调用 HideModeManager → NativeBridge → native_bridge.cpp → ApexFirewall。
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun HideModeScreen(
    onBack: () -> Unit
) {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()
    val isDark = isSystemInDarkTheme()
    val hideManager = remember { HideModeManager(context) }

    var currentMode by remember { mutableStateOf(HideModeManager.MODE_DETECT) }
    var isActive by remember { mutableStateOf(false) }
    var lastError by remember { mutableStateOf("") }
    var switching by remember { mutableStateOf(false) }
    var nativeAvailable by remember { mutableStateOf(true) }
    var toastMessage by remember { mutableStateOf<String?>(null) }

    // 初始化：读取当前模式（包 try-catch 防止 UnsatisfiedLinkError 闪退）
    LaunchedEffect(Unit) {
        withContext(Dispatchers.IO) {
            try {
                currentMode = hideManager.currentMode()
                isActive = hideManager.isActive()
                lastError = hideManager.lastError()
            } catch (e: Throwable) {
                nativeAvailable = false
                lastError = e.message ?: e.javaClass.simpleName
            }
        }
    }

    val snackbarHostState = remember { SnackbarHostState() }

    fun showToast(msg: String) {
        toastMessage = msg
        scope.launch {
            snackbarHostState.showSnackbar(msg)
        }
    }

    fun switchMode(target: Int) {
        if (!nativeAvailable) {
            showToast("原生库未加载，无法切换模式")
            return
        }
        scope.launch {
            switching = true
            withContext(Dispatchers.IO) {
                try {
                    val ok = hideManager.switchToMode(target)
                    currentMode = hideManager.currentMode()
                    isActive = hideManager.isActive()
                    lastError = hideManager.lastError()
                    val modeName = HideModeManager.modeName(target)
                    showToast(if (ok) "$modeName 模式切换成功" else "切换失败: $lastError")
                } catch (e: Throwable) {
                    lastError = e.message ?: e.javaClass.simpleName
                    showToast("切换异常: $lastError")
                }
            }
            switching = false
        }
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = {
                    Text(
                        "隐藏模式",
                        fontWeight = FontWeight.SemiBold,
                        color = TextPrimary
                    )
                },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.Default.ArrowBack, "返回", tint = TextPrimary)
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = Color.Transparent
                )
            )
        },
        snackbarHost = { SnackbarHost(snackbarHostState) },
        containerColor = Color.Transparent
    ) { padding ->
        Column(
            modifier = Modifier
                .padding(padding)
                .fillMaxSize()
                .verticalScroll(rememberScrollState())
                .padding(horizontal = 20.dp, vertical = 16.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            // 状态卡片
            StatusCard(
                currentMode = currentMode,
                isActive = isActive,
                lastError = lastError
            )

            // 模式切换卡片
            Text(
                "模式选择",
                fontSize = 14.sp,
                fontWeight = FontWeight.SemiBold,
                color = TextSecondary
            )
            ModeOptionCard(
                icon = Icons.Default.Search,
                title = "Detection · 检测模式",
                description = "仅扫描设备完整性，不做任何隐藏。日常使用推荐。",
                selected = currentMode == HideModeManager.MODE_DETECT,
                enabled = !switching,
                accentColor = Color(0xFF4CAF50),
                onClick = { switchMode(HideModeManager.MODE_DETECT) }
            )
            ModeOptionCard(
                icon = Icons.Default.VisibilityOff,
                title = "Hide · 隐藏模式",
                description = "对除 APEX-Root 外的所有应用隐藏 root 痕迹。适用于银行 / 风控 App。",
                selected = currentMode == HideModeManager.MODE_HIDE,
                enabled = !switching,
                accentColor = AccentPurple,
                onClick = { switchMode(HideModeManager.MODE_HIDE) }
            )
            ModeOptionCard(
                icon = Icons.Default.SportsEsports,
                title = "Game · 游戏模式",
                description = "激进隐藏 + 性能优化。隐藏 Magisk / KSU / APatch 守护进程，适合游戏反作弊。",
                selected = currentMode == HideModeManager.MODE_GAME,
                enabled = !switching,
                accentColor = Color(0xFFFF9800),
                onClick = { switchMode(HideModeManager.MODE_GAME) }
            )

            Spacer(modifier = Modifier.height(8.dp))

            // 说明
            Card(
                modifier = Modifier.fillMaxWidth(),
                shape = RoundedCornerShape(16.dp),
                colors = CardDefaults.cardColors(containerColor = Color.Transparent)
            ) {
                Column(
                    modifier = Modifier
                        .fillMaxWidth()
                        .clip(RoundedCornerShape(16.dp))
                        .liquidGlass(
                            cornerRadius = 16.dp,
                            baseColor = if (isDark) DeepSurface.copy(alpha = 0.5f) else LightSurface.copy(alpha = 0.5f)
                        )
                        .padding(16.dp),
                    verticalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Icon(
                            Icons.Default.Info,
                            contentDescription = null,
                            tint = AccentPurple,
                            modifier = Modifier.size(16.dp)
                        )
                        Spacer(Modifier.width(8.dp))
                        Text(
                            "工作原理",
                            fontSize = 14.sp,
                            fontWeight = FontWeight.SemiBold,
                            color = TextPrimary
                        )
                    }
                    Text(
                        "• Android 12+ (kernel ≥5.10)：使用 eBPF 防火墙拦截 openat/statx/getdents/access 系统调用\n" +
                        "• Android 10-11：使用 mount namespace 隔离 + LD_PRELOAD libc 拦截\n" +
                        "• 白名单：APEX-Root 自身 UID 始终可见真实状态\n" +
                        "• 模式切换实时生效，无需重启",
                        fontSize = 12.sp,
                        color = TextSecondary,
                        lineHeight = 18.sp
                    )
                }
            }

            // 紧急停止按钮
            if (isActive) {
                Button(
                    onClick = {
                        scope.launch {
                            switching = true
                            withContext(Dispatchers.IO) {
                                try {
                                    hideManager.stopHideMode()
                                    currentMode = hideManager.currentMode()
                                    isActive = hideManager.isActive()
                                    lastError = hideManager.lastError()
                                    showToast("已停止隐藏")
                                } catch (e: Throwable) {
                                    lastError = e.message ?: e.javaClass.simpleName
                                    showToast("停止失败: $lastError")
                                }
                            }
                            switching = false
                        }
                    },
                    enabled = !switching,
                    modifier = Modifier.fillMaxWidth(),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = Color(0xFFE53935)
                    ),
                    shape = RoundedCornerShape(12.dp)
                ) {
                    Icon(Icons.Default.Stop, contentDescription = null)
                    Spacer(Modifier.width(8.dp))
                    Text("紧急停止", fontWeight = FontWeight.Medium)
                }
            }

            // 原生库不可用警告
            if (!nativeAvailable) {
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    shape = RoundedCornerShape(12.dp),
                    colors = CardDefaults.cardColors(
                        containerColor = Color(0xFFE53935).copy(alpha = 0.1f)
                    )
                ) {
                    Row(
                        modifier = Modifier.padding(16.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Icon(
                            Icons.Default.Warning,
                            contentDescription = null,
                            tint = Color(0xFFE53935),
                            modifier = Modifier.size(24.dp)
                        )
                        Spacer(Modifier.width(12.dp))
                        Column {
                            Text(
                                "原生库未加载",
                                fontSize = 14.sp,
                                fontWeight = FontWeight.SemiBold,
                                color = Color(0xFFE53935)
                            )
                            Text(
                                "隐藏模式需要原生库支持。请确认应用已正确安装且设备已 root。",
                                fontSize = 12.sp,
                                color = TextSecondary
                            )
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun StatusCard(
    currentMode: Int,
    isActive: Boolean,
    lastError: String
) {
    val isDark = isSystemInDarkTheme()
    val statusColor = when {
        lastError.isNotEmpty() -> Color(0xFFE53935)
        isActive -> Color(0xFFFF9800)
        else -> Color(0xFF4CAF50)
    }
    val statusText = when {
        lastError.isNotEmpty() -> "错误"
        isActive -> "已激活"
        else -> "空闲"
    }
    val modeText = HideModeManager.modeName(currentMode)

    // 修复：深色模式用 DeepSurface，浅色模式用 LightSurface，避免深色模式泛白
    val glassBaseColor = if (isDark) DeepSurface.copy(alpha = 0.6f) else LightSurface.copy(alpha = 0.7f)

    Card(
        modifier = Modifier.fillMaxWidth(),
        shape = RoundedCornerShape(20.dp),
        colors = CardDefaults.cardColors(containerColor = Color.Transparent)
    ) {
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .clip(RoundedCornerShape(20.dp))
                .liquidGlass(
                    cornerRadius = 20.dp,
                    baseColor = glassBaseColor
                )
                .padding(20.dp)
        ) {
            Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Column {
                        Text(
                            "当前状态",
                            fontSize = 12.sp,
                            color = TextTertiary
                        )
                        Text(
                            statusText,
                            fontSize = 24.sp,
                            fontWeight = FontWeight.Bold,
                            color = statusColor
                        )
                    }
                    Box(
                        modifier = Modifier
                            .size(12.dp)
                            .clip(RoundedCornerShape(50))
                            .background(statusColor)
                    )
                }
                Divider(color = TextTertiary.copy(alpha = 0.2f))
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Text("当前模式", fontSize = 12.sp, color = TextTertiary)
                    Text(modeText, fontSize = 14.sp, fontWeight = FontWeight.Medium, color = TextPrimary)
                }
                if (lastError.isNotEmpty()) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Text("最近错误", fontSize = 12.sp, color = TextTertiary)
                        Text(
                            lastError,
                            fontSize = 12.sp,
                            color = Color(0xFFE53935),
                            fontWeight = FontWeight.Medium
                        )
                    }
                }
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun ModeOptionCard(
    icon: ImageVector,
    title: String,
    description: String,
    selected: Boolean,
    enabled: Boolean,
    accentColor: Color,
    onClick: () -> Unit
) {
    val scale by animateFloatAsState(
        targetValue = if (selected) 1f else 0.98f,
        animationSpec = tween(200),
        label = "mode_scale"
    )

    Card(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(16.dp)),
        shape = RoundedCornerShape(16.dp),
        colors = CardDefaults.cardColors(
            containerColor = if (selected) accentColor.copy(alpha = 0.08f) else Color.Transparent
        ),
        onClick = onClick,
        enabled = enabled
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Box(
                modifier = Modifier
                    .size(44.dp)
                    .clip(RoundedCornerShape(12.dp))
                    .background(accentColor.copy(alpha = 0.15f)),
                contentAlignment = Alignment.Center
            ) {
                Icon(icon, contentDescription = null, tint = accentColor, modifier = Modifier.size(24.dp))
            }
            Spacer(Modifier.width(16.dp))
            Column(modifier = Modifier.weight(1f)) {
                Text(title, fontSize = 15.sp, fontWeight = FontWeight.SemiBold, color = TextPrimary)
                Spacer(Modifier.height(4.dp))
                Text(description, fontSize = 12.sp, color = TextSecondary, lineHeight = 16.sp)
            }
            if (selected) {
                Icon(
                    Icons.Default.CheckCircle,
                    contentDescription = "已选中",
                    tint = accentColor,
                    modifier = Modifier.size(24.dp)
                )
            }
        }
    }
}
