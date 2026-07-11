package com.apex.root.ui.compose.screens

import android.Manifest
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.os.Environment
import android.provider.Settings
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
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
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.compose.ui.platform.LocalLifecycleOwner
import com.apex.root.core.permission.PermissionManager
import com.apex.root.core.permission.PermissionManager.PermissionInfo
import com.apex.root.core.permission.PermissionManager.PermState
import com.apex.root.core.permission.PermissionManager.PermType
import com.apex.root.ui.compose.*
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * 权限中心 / 统一权限管理页。
 *
 * 集中展示本应用所需的所有权限及其授权状态，并支持：
 *  - 运行时权限（通知 / 存储≤Android12）：直接在应用内弹出系统授权对话框
 *  - 特殊权限（所有文件访问 / 无障碍 / 悬浮窗 / 使用情况）：跳转对应系统设置页
 *  - ROOT 权限：展示 su 检测结果（无法通过系统 API 申请，由 root 框架授权）
 *
 * 设计参考：
 *  - RikkaApps/AppOps 的权限分组卡片
 *  - MuntashirAkon/AppManager 的权限状态徽章
 *  - topjohnwu/Magisk 的 root 授权引导
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun PermissionsScreen(onBack: () -> Unit) {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()
    val scrollBehavior = TopAppBarDefaults.exitUntilCollapsedScrollBehavior()
    val isDark = LocalIsDarkTheme.current

    var permissions by remember { mutableStateOf<List<PermissionInfo>>(emptyList()) }
    var loading by remember { mutableStateOf(true) }

    fun load() {
        scope.launch {
            loading = true
            try {
                val list = withContext(Dispatchers.IO) { PermissionManager.checkAll(context) }
                permissions = list
            } catch (e: Throwable) {
                // 防御性 try-catch：任何异常（如 ClosedSendException 在 composition dispose 期间，
                // 或 NPE 在 list mapping 中）都会传播到 scope 的 CoroutineExceptionHandler，
                // 在 main scope 中会闪退整个 App。此处兜底避免闪退。
                android.util.Log.e("PermissionsScreen", "load failed", e)
            } finally {
                loading = false
            }
        }
    }

    // 首次加载 + 每次 onResume（从设置页返回）自动刷新
    LaunchedEffect(Unit) { load() }
    val lifecycleOwner = LocalLifecycleOwner.current
    DisposableEffect(lifecycleOwner) {
        val observer = LifecycleEventObserver { _, event ->
            if (event == Lifecycle.Event.ON_RESUME) {
                load()
            }
        }
        lifecycleOwner.lifecycle.addObserver(observer)
        onDispose { lifecycleOwner.lifecycle.removeObserver(observer) }
    }

    // 运行时权限请求 Launcher（通知 + 存储）
    val runtimePermLauncher = rememberLauncherForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { _ -> load() }

    fun requestRuntime(perms: Array<String>) {
        runtimePermLauncher.launch(perms)
    }

    fun openSettings(action: String, data: Uri? = null) {
        runCatching {
            // 注意：不用 Intent(action).apply { data = ... }，因为 Intent 自身有 data 属性会遮蔽外层参数
            val intent = Intent(action)
            if (data != null) intent.data = data
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            context.startActivity(intent)
        }
    }

    val grantedCount = permissions.count { it.state == PermState.GRANTED }
    val totalCount = permissions.size

    Scaffold(
        modifier = Modifier.nestedScroll(scrollBehavior.nestedScrollConnection),
        containerColor = Color.Transparent,
        topBar = {
            CollapsibleGlassTopBar(
                title = "权限中心",
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
            fluidColorsDark = PageFluidColors.dashboard,
            fluidColorsLight = PageFluidColors.dashboardLight
        ) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(padding)
                    .verticalScroll(rememberScrollState())
                    .padding(horizontal = 16.dp, vertical = 8.dp),
                verticalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                // ── 顶部汇总卡片 ──
                PermissionSummaryCard(
                    granted = grantedCount,
                    total = totalCount,
                    loading = loading
                )

                // ── 权限列表 ──
                permissions.forEach { info ->
                    PermissionCard(
                        info = info,
                        onRequest = { handleRequest(info, ::requestRuntime, ::openSettings, context) }
                    )
                }

                // ── 说明区 ──
                Box(
                    Modifier
                        .fillMaxWidth()
                        .background(
                            if (isDark) Color.White.copy(alpha = 0.03f) else Color.Black.copy(alpha = 0.02f),
                            RoundedCornerShape(12.dp)
                        )
                        .padding(14.dp)
                ) {
                    Text(
                        "提示：ROOT 权限由已安装的 root 框架（Magisk / KernelSU / APatch）授权，" +
                            "无法通过系统 API 申请。无障碍 / 悬浮窗 / 所有文件访问 / 使用情况访问" +
                            "属于特殊权限，需跳转系统设置页手动开启。返回本页将自动刷新状态。",
                        fontSize = 11.sp,
                        color = TextTertiary,
                        lineHeight = 17.sp
                    )
                }

                Spacer(Modifier.height(24.dp))
            }
        }
    }
}

/** 顶部汇总：已授权数量 / 总数 + 进度环 */
@Composable
private fun PermissionSummaryCard(granted: Int, total: Int, loading: Boolean) {
    val isDark = LocalIsDarkTheme.current
    val ratio = if (total > 0) granted.toFloat() / total else 0f
    val allGranted = granted == total && total > 0

    GlassCard(cornerRadius = 22.dp) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically
        ) {
            // 进度环
            Box(
                modifier = Modifier.size(64.dp),
                contentAlignment = Alignment.Center
            ) {
                CircularProgressIndicator(
                    progress = if (loading) 0f else ratio,
                    modifier = Modifier.fillMaxSize(),
                    strokeWidth = 5.dp,
                    color = if (allGranted) AccentMint else AccentPurple,
                    trackColor = if (isDark) Color.White.copy(alpha = 0.08f) else Color.Black.copy(alpha = 0.06f)
                )
                Text(
                    if (loading) "…" else "$granted/$total",
                    fontSize = 13.sp,
                    fontWeight = FontWeight.Bold,
                    color = TextPrimary
                )
            }
            Spacer(Modifier.width(18.dp))
            Column(Modifier.weight(1f)) {
                Text(
                    if (allGranted) "全部权限已就绪" else "部分权限待授权",
                    fontSize = 16.sp,
                    fontWeight = FontWeight.SemiBold,
                    color = TextPrimary
                )
                Spacer(Modifier.height(4.dp))
                Text(
                    if (loading) "正在检测权限状态…" else "完整授权后可使用全部检测与防护功能",
                    fontSize = 12.sp,
                    color = TextSecondary
                )
            }
            if (!loading) {
                Icon(
                    if (allGranted) Icons.Default.Verified else Icons.Default.Warning,
                    contentDescription = null,
                    tint = if (allGranted) AccentMint else AccentGold,
                    modifier = Modifier.size(26.dp)
                )
            }
        }
    }
}

/** 单个权限卡片 */
@Composable
private fun PermissionCard(info: PermissionInfo, onRequest: () -> Unit) {
    val isDark = LocalIsDarkTheme.current
    val (icon, iconTint, title) = permissionMeta(info.type)

    val stateBadge = when (info.state) {
        PermState.GRANTED -> Triple("已授权", AccentMint, Icons.Default.CheckCircle)
        PermState.DENIED -> Triple("未授权", AccentGold, Icons.Default.Warning)
        PermState.SPECIAL -> Triple("需手动开启", AccentGold, Icons.Default.Settings)
        PermState.UNAVAILABLE -> Triple("不可用", TextTertiary, Icons.Default.Block)
    }

    val actionLabel = when (info.state) {
        PermState.GRANTED -> null
        PermState.DENIED -> "去授权"
        PermState.SPECIAL -> "去设置"
        PermState.UNAVAILABLE -> null
    }

    GlassCard(cornerRadius = 18.dp, accentLine = iconTint) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            // 图标
            Box(
                modifier = Modifier
                    .size(42.dp)
                    .clip(RoundedCornerShape(12.dp))
                    .background(
                        Brush.linearGradient(listOf(iconTint.copy(alpha = 0.22f), iconTint.copy(alpha = 0.06f)))
                    ),
                contentAlignment = Alignment.Center
            ) {
                Icon(icon, contentDescription = null, tint = iconTint, modifier = Modifier.size(22.dp))
            }
            Spacer(Modifier.width(14.dp))
            // 标题 + 详情
            Column(Modifier.weight(1f)) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text(title, fontSize = 14.sp, fontWeight = FontWeight.SemiBold, color = TextPrimary)
                    Spacer(Modifier.width(8.dp))
                    // 状态徽章
                    Row(
                        verticalAlignment = Alignment.CenterVertically,
                        modifier = Modifier
                            .clip(RoundedCornerShape(8.dp))
                            .background(stateBadge.second.copy(alpha = 0.14f))
                            .padding(horizontal = 7.dp, vertical = 2.dp)
                    ) {
                        Icon(
                            stateBadge.third,
                            contentDescription = null,
                            tint = stateBadge.second,
                            modifier = Modifier.size(11.dp)
                        )
                        Spacer(Modifier.width(3.dp))
                        Text(stateBadge.first, fontSize = 10.sp, color = stateBadge.second, fontWeight = FontWeight.Medium)
                    }
                }
                Spacer(Modifier.height(3.dp))
                Text(info.detail, fontSize = 11.sp, color = TextTertiary, lineHeight = 15.sp)
            }
            // 操作按钮
            if (actionLabel != null) {
                Spacer(Modifier.width(8.dp))
                Button(
                    onClick = onRequest,
                    contentPadding = PaddingValues(horizontal = 14.dp, vertical = 8.dp),
                    shape = RoundedCornerShape(12.dp),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = iconTint.copy(alpha = 0.85f),
                        contentColor = Color.White
                    )
                ) {
                    Text(actionLabel, fontSize = 12.sp, fontWeight = FontWeight.SemiBold)
                }
            }
        }
    }
}

/** 权限元数据（图标 / 颜色 / 标题） */
private fun permissionMeta(type: PermType): Triple<ImageVector, Color, String> = when (type) {
    PermType.ROOT -> Triple(Icons.Default.AdminPanelSettings, AccentPurple, "ROOT 权限")
    PermType.NOTIFICATION -> Triple(Icons.Default.Notifications, AccentBlue, "通知权限")
    PermType.STORAGE -> Triple(Icons.Default.Folder, AccentGold, "存储 / 日志导出")
    PermType.PHONE_STATE -> Triple(Icons.Default.PhoneAndroid, AccentMint, "读取手机信息")
    PermType.ACCESSIBILITY -> Triple(Icons.Default.AccessibilityNew, AccentMint, "无障碍服务")
    PermType.OVERLAY -> Triple(Icons.Default.Layers, AccentCyan, "悬浮窗权限")
    PermType.USAGE_STATS -> Triple(Icons.Default.Insights, AccentPurpleSoft, "使用情况访问")
}

/**
 * 根据权限类型决定请求方式：
 *  - ROOT：无法通过 API 申请，引导安装 root 框架（打开 Magisk GitHub）
 *  - NOTIFICATION：Android 13+ 运行时弹窗
 *  - STORAGE（≤Android12）：运行时弹窗；（≥Android13）：跳所有文件访问设置
 *  - 其余特殊权限：跳对应系统设置页
 */
private fun handleRequest(
    info: PermissionInfo,
    requestRuntime: (Array<String>) -> Unit,
    openSettings: (String, Uri?) -> Unit,
    context: android.content.Context
) {
    when (info.type) {
        PermType.ROOT -> {
            // 引导安装 root 框架
            runCatching {
                val intent = Intent(Intent.ACTION_VIEW, Uri.parse("https://github.com/topjohnwu/Magisk")).apply {
                    addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                }
                context.startActivity(intent)
            }
        }
        PermType.NOTIFICATION -> {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                requestRuntime(arrayOf(Manifest.permission.POST_NOTIFICATIONS))
            }
        }
        PermType.STORAGE -> {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                // Android 11+：跳所有文件访问权限页
                runCatching {
                    val intent = Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION).apply {
                        data = Uri.parse("package:${context.packageName}")
                        addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                    }
                    context.startActivity(intent)
                }.onFailure {
                    // 回退到通用所有文件访问页
                    openSettings(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION, null)
                }
            } else {
                requestRuntime(arrayOf(Manifest.permission.READ_EXTERNAL_STORAGE))
            }
        }
        PermType.PHONE_STATE -> {
            // v1.0.7: 读取手机信息 — 运行时弹窗
            requestRuntime(arrayOf(Manifest.permission.READ_PHONE_STATE))
        }
        PermType.ACCESSIBILITY -> {
            openSettings(Settings.ACTION_ACCESSIBILITY_SETTINGS, null)
        }
        PermType.OVERLAY -> {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                runCatching {
                    val intent = Intent(
                        Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                        Uri.parse("package:${context.packageName}")
                    ).apply { addFlags(Intent.FLAG_ACTIVITY_NEW_TASK) }
                    context.startActivity(intent)
                }
            }
        }
        PermType.USAGE_STATS -> {
            openSettings(Settings.ACTION_USAGE_ACCESS_SETTINGS, null)
        }
    }
}
