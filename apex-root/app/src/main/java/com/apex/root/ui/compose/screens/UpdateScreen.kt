package com.apex.root.ui.compose.screens

import android.content.Intent
import android.net.Uri
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
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.apex.root.core.NativeLibraryLoader
import com.apex.root.data.updater.DownloadState
import com.apex.root.data.updater.ModuleZipInfo
import com.apex.root.ui.compose.*
import com.apex.root.viewmodel.ModuleCheckState
import com.apex.root.viewmodel.UpdateUiState
import com.apex.root.viewmodel.UpdateViewModel
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun UpdateScreen(
    viewModel: UpdateViewModel,
    onBack: () -> Unit
) {
    val scrollBehavior = TopAppBarDefaults.exitUntilCollapsedScrollBehavior()
    val isDark = LocalIsDarkTheme.current
    val uiState by viewModel.uiState.collectAsState()
    val downloadState by viewModel.downloadState.collectAsState()
    val moduleCheckState by viewModel.moduleCheckState.collectAsState()
    val moduleDownloadState by viewModel.moduleDownloadState.collectAsState()
    val context = LocalContext.current
    val scope = rememberCoroutineScope()

    // Native 库加载状态（本地 mutableState，因为 NativeLibraryLoader 不是 StateFlow）
    var nativeLibLoaded by remember { mutableStateOf(NativeLibraryLoader.isAvailable) }
    var nativeLibFailed by remember { mutableStateOf(NativeLibraryLoader.loadFailed) }
    var nativeLibRetrying by remember { mutableStateOf(false) }

    Scaffold(
        modifier = Modifier.nestedScroll(scrollBehavior.nestedScrollConnection),
        containerColor = Color.Transparent,
        topBar = {
            CollapsibleGlassTopBar(
                title = "软件更新",
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
                verticalArrangement = Arrangement.spacedBy(14.dp)
            ) {
                // ─── 当前版本信息卡片 ───
                CurrentVersionCard(
                    version = viewModel.currentVersion,
                    isDark = isDark
                )

                // ─── 检查更新按钮 ───
                Button(
                    onClick = { viewModel.checkForUpdates() },
                    modifier = Modifier.fillMaxWidth().height(52.dp),
                    shape = RoundedCornerShape(14.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentPurple),
                    enabled = uiState !is UpdateUiState.Checking
                ) {
                    if (uiState is UpdateUiState.Checking) {
                        CircularProgressIndicator(
                            modifier = Modifier.size(20.dp),
                            color = Color.White,
                            strokeWidth = 2.dp
                        )
                        Spacer(Modifier.width(12.dp))
                        Text("正在检查...", color = Color.White, fontWeight = FontWeight.SemiBold)
                    } else {
                        Icon(Icons.Default.Refresh, null, Modifier.size(20.dp), tint = Color.White)
                        Spacer(Modifier.width(8.dp))
                        Text("检查更新", color = Color.White, fontWeight = FontWeight.SemiBold)
                    }
                }

                // ─── 检查结果区 ───
                when (val state = uiState) {
                    is UpdateUiState.Idle -> {
                        InfoHintCard(
                            icon = Icons.Default.Info,
                            text = "点击上方按钮检查最新版本",
                            isDark = isDark
                        )
                    }
                    is UpdateUiState.Checking -> {
                        // 按钮已显示进度，这里不重复
                    }
                    is UpdateUiState.UpToDate -> {
                        StatusCard(
                            icon = Icons.Default.CheckCircle,
                            iconTint = AccentMint,
                            title = "已是最新版本",
                            subtitle = "当前版本 v${state.currentVersion}，无需更新",
                            isDark = isDark
                        )
                    }
                    is UpdateUiState.NetworkNotSatisfied -> {
                        StatusCard(
                            icon = Icons.Default.WifiOff,
                            iconTint = AccentGold,
                            title = "网络条件不满足",
                            subtitle = "已设置为仅 Wi-Fi 下检查更新，请连接 Wi-Fi 后重试",
                            isDark = isDark
                        )
                    }
                    is UpdateUiState.CheckFailed -> {
                        StatusCard(
                            icon = Icons.Default.ErrorOutline,
                            iconTint = ErrorRed,
                            title = "检查更新失败",
                            subtitle = state.message,
                            isDark = isDark,
                            actionLabel = "在浏览器中打开",
                            onAction = {
                                runCatching {
                                    context.startActivity(Intent(Intent.ACTION_VIEW,
                                        Uri.parse("https://github.com/mengjinghao/root-check/releases")).apply {
                                        addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                                    })
                                }
                            }
                        )
                    }
                    is UpdateUiState.UpdateAvailable -> {
                        UpdateAvailableCard(
                            state = state,
                            downloadState = downloadState,
                            onDownload = { viewModel.downloadUpdate() },
                            onCancel = { viewModel.cancelDownload() },
                            onInstall = { viewModel.installDownloadedApk() },
                            onOpenInBrowser = { viewModel.openReleaseInBrowser() },
                            isDark = isDark
                        )
                    }
                }

                Spacer(Modifier.height(16.dp))

                // ─── Magisk 模块下载区 ───
                SectionHeader(title = "Magisk 模块")
                Spacer(Modifier.height(8.dp))
                ModuleDownloadCard(
                    moduleCheckState = moduleCheckState,
                    moduleDownloadState = moduleDownloadState,
                    onCheck = { viewModel.checkForModuleZip() },
                    onDownload = { viewModel.downloadModuleZip() },
                    onCancel = { viewModel.cancelModuleDownload() },
                    onInstall = { viewModel.installDownloadedModuleZip() },
                    onOpenInBrowser = {
                        runCatching {
                            context.startActivity(Intent(Intent.ACTION_VIEW,
                                Uri.parse("https://github.com/mengjinghao/root-check/releases")).apply {
                                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                            })
                        }
                    },
                    isDark = isDark
                )

                Spacer(Modifier.height(16.dp))

                // ─── Native 库诊断区 ───
                SectionHeader(title = "原生引擎诊断")
                Spacer(Modifier.height(8.dp))
                NativeLibDiagnosticCard(
                    loaded = nativeLibLoaded,
                    failed = nativeLibFailed,
                    retrying = nativeLibRetrying,
                    onRetry = {
                        if (!nativeLibRetrying) {
                            nativeLibRetrying = true
                            scope.launch {
                                withContext(Dispatchers.IO) {
                                    NativeLibraryLoader.reset()
                                    NativeLibraryLoader.ensureLoaded()
                                }
                                nativeLibLoaded = NativeLibraryLoader.isAvailable
                                nativeLibFailed = NativeLibraryLoader.loadFailed
                                nativeLibRetrying = false
                            }
                        }
                    },
                    isDark = isDark
                )

                Spacer(Modifier.height(16.dp))

                // ─── 更新说明 ───
                SectionHeader(title = "更新说明")
                Spacer(Modifier.height(8.dp))
                GlassCard(cornerRadius = 14.dp) {
                    Column(modifier = Modifier.padding(16.dp)) {
                        Text(
                            "• 本应用通过 GitHub Releases 检查更新，需联网访问 github.com",
                            fontSize = 11.sp,
                            color = TextSecondary
                        )
                        Spacer(Modifier.height(6.dp))
                        Text(
                            "• 默认仅在 Wi-Fi 下下载 APK，避免消耗移动流量",
                            fontSize = 11.sp,
                            color = TextSecondary
                        )
                        Spacer(Modifier.height(6.dp))
                        Text(
                            "• APK 下载到应用缓存目录，安装时由系统包安装器接管",
                            fontSize = 11.sp,
                            color = TextSecondary
                        )
                        Spacer(Modifier.height(6.dp))
                        Text(
                            "• 更新通道可在「设置 → 更新」中切换：稳定版 / Beta / 开发版",
                            fontSize = 11.sp,
                            color = TextSecondary
                        )
                        Spacer(Modifier.height(6.dp))
                        Text(
                            "• 如自动下载失败，可点击「在浏览器中打开」手动下载",
                            fontSize = 11.sp,
                            color = TextSecondary
                        )
                    }
                }

                Spacer(Modifier.height(24.dp))

                // ─── 关于本应用 ───
                SectionHeader(title = "关于")
                Spacer(Modifier.height(8.dp))
                GlassCard(cornerRadius = 14.dp) {
                    Column(modifier = Modifier.padding(16.dp)) {
                        InfoRow(label = "应用名称", value = "APEX Root")
                        InfoRow(label = "当前版本", value = "v${viewModel.currentVersion}")
                        InfoRow(label = "更新源", value = "GitHub Releases")
                        InfoRow(label = "仓库地址", value = "mengjinghao/root-check")
                    }
                }
            }
        }
    }
}

// ─── 子组件 ──────────────────────────────────

@Composable
private fun CurrentVersionCard(version: String, isDark: Boolean) {
    GlassCard(cornerRadius = 16.dp, accentLine = AccentPurple) {
        Row(
            modifier = Modifier.fillMaxWidth().padding(20.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Box(
                modifier = Modifier
                    .size(56.dp)
                    .background(AccentPurple.copy(alpha = 0.15f), CircleShape),
                contentAlignment = Alignment.Center
            ) {
                Icon(
                    Icons.Default.Security,
                    contentDescription = null,
                    tint = AccentPurple,
                    modifier = Modifier.size(32.dp)
                )
            }
            Spacer(Modifier.width(16.dp))
            Column {
                Text(
                    "APEX Root",
                    fontSize = 18.sp,
                    fontWeight = FontWeight.Bold,
                    color = if (isDark) TextPrimary else Color(0xFF1A1A2E)
                )
                Text(
                    "当前版本: v$version",
                    fontSize = 13.sp,
                    color = TextSecondary
                )
            }
        }
    }
}

@Composable
private fun StatusCard(
    icon: androidx.compose.ui.graphics.vector.ImageVector,
    iconTint: Color,
    title: String,
    subtitle: String,
    isDark: Boolean,
    actionLabel: String? = null,
    onAction: (() -> Unit)? = null
) {
    GlassCard(cornerRadius = 14.dp, accentLine = iconTint) {
        Column(modifier = Modifier.padding(16.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Icon(icon, contentDescription = null, tint = iconTint, modifier = Modifier.size(28.dp))
                Spacer(Modifier.width(12.dp))
                Text(
                    title,
                    fontSize = 16.sp,
                    fontWeight = FontWeight.Bold,
                    color = iconTint
                )
            }
            Spacer(Modifier.height(8.dp))
            Text(
                subtitle,
                fontSize = 12.sp,
                color = TextSecondary
            )
            if (actionLabel != null && onAction != null) {
                Spacer(Modifier.height(12.dp))
                OutlinedButton(
                    onClick = onAction,
                    shape = RoundedCornerShape(10.dp)
                ) {
                    Text(actionLabel, fontSize = 12.sp)
                }
            }
        }
    }
}

@Composable
private fun InfoHintCard(
    icon: androidx.compose.ui.graphics.vector.ImageVector,
    text: String,
    isDark: Boolean
) {
    GlassCard(cornerRadius = 14.dp) {
        Row(
            modifier = Modifier.padding(16.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Icon(icon, contentDescription = null, tint = TextTertiary, modifier = Modifier.size(20.dp))
            Spacer(Modifier.width(10.dp))
            Text(text, fontSize = 12.sp, color = TextSecondary)
        }
    }
}

@Composable
private fun UpdateAvailableCard(
    state: UpdateUiState.UpdateAvailable,
    downloadState: DownloadState,
    onDownload: () -> Unit,
    onCancel: () -> Unit,
    onInstall: () -> Unit,
    onOpenInBrowser: () -> Unit,
    isDark: Boolean
) {
    GlassCard(cornerRadius = 16.dp, accentLine = AccentGold) {
        Column(modifier = Modifier.padding(20.dp)) {
            // 标题
            Row(verticalAlignment = Alignment.CenterVertically) {
                Icon(
                    Icons.Default.SystemUpdateAlt,
                    contentDescription = null,
                    tint = AccentGold,
                    modifier = Modifier.size(28.dp)
                )
                Spacer(Modifier.width(12.dp))
                Column {
                    Text(
                        "发现新版本",
                        fontSize = 18.sp,
                        fontWeight = FontWeight.Bold,
                        color = if (isDark) TextPrimary else Color(0xFF1A1A2E)
                    )
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Text(
                            "v${state.currentVersion} → v${state.newVersion}",
                            fontSize = 13.sp,
                            color = TextSecondary,
                            fontFamily = FontFamily.Monospace
                        )
                        if (state.isPrerelease) {
                            Spacer(Modifier.width(8.dp))
                            Box(
                                modifier = Modifier
                                    .background(AccentGold.copy(alpha = 0.2f), RoundedCornerShape(4.dp))
                                    .padding(horizontal = 6.dp, vertical = 2.dp)
                            ) {
                                Text("预发布", fontSize = 10.sp, color = AccentGold, fontWeight = FontWeight.Bold)
                            }
                        }
                    }
                }
            }

            Spacer(Modifier.height(12.dp))

            // 发布信息
            if (state.publishedAt.isNotEmpty()) {
                val formattedDate = runCatching {
                    val sdf = SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss'Z'", Locale.US)
                    val date = sdf.parse(state.publishedAt)
                    SimpleDateFormat("yyyy-MM-dd HH:mm", Locale.getDefault()).format(date ?: Date())
                }.getOrDefault(state.publishedAt)
                Text("发布时间: $formattedDate", fontSize = 11.sp, color = TextTertiary)
            }
            if (state.apkSizeBytes > 0) {
                Text(
                    "APK 大小: ${formatFileSize(state.apkSizeBytes)}",
                    fontSize = 11.sp,
                    color = TextTertiary
                )
            }

            Spacer(Modifier.height(14.dp))

            // 更新日志
            if (state.releaseNotes.isNotEmpty()) {
                Text(
                    "更新日志",
                    fontSize = 13.sp,
                    fontWeight = FontWeight.SemiBold,
                    color = if (isDark) TextPrimary else Color(0xFF1A1A2E)
                )
                Spacer(Modifier.height(6.dp))
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .background(
                            if (isDark) DeepSurfaceVariant else Color(0xFFF1F5F9),
                            RoundedCornerShape(8.dp)
                        )
                        .padding(12.dp)
                ) {
                    Text(
                        state.releaseNotes.take(2000),  // 限制最大显示长度
                        fontSize = 11.sp,
                        color = TextSecondary,
                        fontFamily = FontFamily.Monospace
                    )
                }
            }

            Spacer(Modifier.height(16.dp))

            // 操作按钮区
            when (downloadState) {
                is DownloadState.Idle -> {
                    Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                        Button(
                            onClick = onDownload,
                            modifier = Modifier.weight(1f).height(46.dp),
                            shape = RoundedCornerShape(12.dp),
                            colors = ButtonDefaults.buttonColors(containerColor = AccentPurple)
                        ) {
                            Icon(Icons.Default.Download, null, Modifier.size(18.dp), tint = Color.White)
                            Spacer(Modifier.width(6.dp))
                            Text("下载并安装", color = Color.White, fontSize = 13.sp, fontWeight = FontWeight.SemiBold)
                        }
                        OutlinedButton(
                            onClick = onOpenInBrowser,
                            modifier = Modifier.height(46.dp),
                            shape = RoundedCornerShape(12.dp)
                        ) {
                            Text("浏览器打开", fontSize = 12.sp)
                        }
                    }
                }
                is DownloadState.Downloading -> {
                    Column {
                        // 兼容 Compose Material3 1.1.x：使用 Float 参数的旧式 LinearProgressIndicator
                        @Suppress("DEPRECATION")
                        LinearProgressIndicator(
                            progress = downloadState.progress / 100f,
                            modifier = Modifier.fillMaxWidth().height(8.dp),
                            color = AccentPurple,
                            trackColor = if (isDark) DeepSurfaceVariant else Color(0xFFE2E8F0)
                        )
                        Spacer(Modifier.height(6.dp))
                        Row(
                            modifier = Modifier.fillMaxWidth(),
                            horizontalArrangement = Arrangement.SpaceBetween,
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            Text(
                                "${downloadState.progress}%  ·  ${formatFileSize(downloadState.downloadedBytes)} / ${formatFileSize(downloadState.totalBytes)}",
                                fontSize = 11.sp,
                                color = TextSecondary,
                                fontFamily = FontFamily.Monospace
                            )
                            OutlinedButton(
                                onClick = onCancel,
                                modifier = Modifier.height(34.dp),
                                shape = RoundedCornerShape(8.dp),
                                contentPadding = PaddingValues(horizontal = 12.dp, vertical = 0.dp)
                            ) {
                                Text("取消", fontSize = 11.sp, color = ErrorRed)
                            }
                        }
                    }
                }
                is DownloadState.Completed -> {
                    Button(
                        onClick = onInstall,
                        modifier = Modifier.fillMaxWidth().height(46.dp),
                        shape = RoundedCornerShape(12.dp),
                        colors = ButtonDefaults.buttonColors(containerColor = AccentMint)
                    ) {
                        Icon(Icons.Default.InstallMobile, null, Modifier.size(18.dp), tint = Color.White)
                        Spacer(Modifier.width(8.dp))
                        Text("点击安装 v${state.newVersion}", color = Color.White, fontSize = 13.sp, fontWeight = FontWeight.SemiBold)
                    }
                    Spacer(Modifier.height(8.dp))
                    Text(
                        "APK 已下载完成，点击上方按钮启动系统安装器",
                        fontSize = 11.sp,
                        color = TextTertiary
                    )
                }
                is DownloadState.Failed -> {
                    StatusCard(
                        icon = Icons.Default.ErrorOutline,
                        iconTint = ErrorRed,
                        title = "下载失败",
                        subtitle = downloadState.message,
                        isDark = isDark,
                        actionLabel = "重试",
                        onAction = onDownload
                    )
                }
                is DownloadState.Cancelled -> {
                    Text("下载已取消", fontSize = 12.sp, color = TextTertiary)
                    Spacer(Modifier.height(8.dp))
                    Button(
                        onClick = onDownload,
                        modifier = Modifier.fillMaxWidth().height(46.dp),
                        shape = RoundedCornerShape(12.dp),
                        colors = ButtonDefaults.buttonColors(containerColor = AccentPurple)
                    ) {
                        Text("重新下载", color = Color.White, fontSize = 13.sp)
                    }
                }
            }
        }
    }
}

@Composable
private fun InfoRow(label: String, value: String) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp),
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(label, fontSize = 12.sp, color = TextSecondary)
        Text(value, fontSize = 12.sp, color = TextPrimary, fontFamily = FontFamily.Monospace)
    }
}

private fun formatFileSize(bytes: Long): String {
    if (bytes <= 0) return "0 B"
    val units = arrayOf("B", "KB", "MB", "GB")
    var size = bytes.toDouble()
    var unitIndex = 0
    while (size >= 1024 && unitIndex < units.size - 1) {
        size /= 1024
        unitIndex++
    }
    return if (unitIndex == 0) "${bytes.toInt()} ${units[unitIndex]}"
    else String.format("%.1f %s", size, units[unitIndex])
}

// ─── Magisk 模块下载卡片 ──────────────────────────

@Composable
private fun ModuleDownloadCard(
    moduleCheckState: ModuleCheckState,
    moduleDownloadState: DownloadState,
    onCheck: () -> Unit,
    onDownload: () -> Unit,
    onCancel: () -> Unit,
    onInstall: () -> Unit,
    onOpenInBrowser: () -> Unit,
    isDark: Boolean
) {
    GlassCard(cornerRadius = 16.dp, accentLine = AccentBlue) {
        Column(modifier = Modifier.padding(20.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Icon(
                    Icons.Default.Extension,
                    contentDescription = null,
                    tint = AccentBlue,
                    modifier = Modifier.size(28.dp)
                )
                Spacer(Modifier.width(12.dp))
                Column(modifier = Modifier.weight(1f)) {
                    Text(
                        "APEX-Root Magisk 模块",
                        fontSize = 16.sp,
                        fontWeight = FontWeight.Bold,
                        color = if (isDark) TextPrimary else Color(0xFF1A1A2E)
                    )
                    Text(
                        "守护进程模块，开机自动隐藏 root",
                        fontSize = 11.sp,
                        color = TextSecondary
                    )
                }
            }

            Spacer(Modifier.height(14.dp))

            // 查询/下载状态区
            when (moduleCheckState) {
                is ModuleCheckState.Idle -> {
                    Text(
                        "点击下方按钮查询 GitHub Releases 中的模块 zip",
                        fontSize = 12.sp,
                        color = TextSecondary
                    )
                    Spacer(Modifier.height(10.dp))
                    OutlinedButton(
                        onClick = onCheck,
                        modifier = Modifier.fillMaxWidth().height(44.dp),
                        shape = RoundedCornerShape(10.dp)
                    ) {
                        Icon(Icons.Default.Search, null, Modifier.size(18.dp))
                        Spacer(Modifier.width(8.dp))
                        Text("查询模块", fontSize = 13.sp)
                    }
                }
                is ModuleCheckState.Checking -> {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        CircularProgressIndicator(
                            modifier = Modifier.size(20.dp),
                            strokeWidth = 2.dp,
                            color = AccentBlue
                        )
                        Spacer(Modifier.width(12.dp))
                        Text("正在查询模块...", fontSize = 12.sp, color = TextSecondary)
                    }
                }
                is ModuleCheckState.NotFound -> {
                    Text(
                        "未找到模块 zip",
                        fontSize = 13.sp,
                        color = AccentGold,
                        fontWeight = FontWeight.Medium
                    )
                    Spacer(Modifier.height(6.dp))
                    Text(
                        "当前 Release 中未上传 Magisk 模块 zip。请发布一个文件名包含 'module' 或 'magisk' 的 .zip 附件。",
                        fontSize = 11.sp,
                        color = TextTertiary
                    )
                    Spacer(Modifier.height(10.dp))
                    OutlinedButton(
                        onClick = onOpenInBrowser,
                        shape = RoundedCornerShape(10.dp)
                    ) {
                        Text("在浏览器中查看 Releases", fontSize = 12.sp)
                    }
                }
                is ModuleCheckState.Error -> {
                    Text(
                        "查询失败: ${moduleCheckState.message}",
                        fontSize = 12.sp,
                        color = ErrorRed
                    )
                    Spacer(Modifier.height(10.dp))
                    OutlinedButton(
                        onClick = onCheck,
                        shape = RoundedCornerShape(10.dp)
                    ) {
                        Text("重试查询", fontSize = 12.sp)
                    }
                }
                is ModuleCheckState.Available -> {
                    val info: ModuleZipInfo = moduleCheckState.info
                    Text(
                        "找到模块: ${info.fileName}",
                        fontSize = 13.sp,
                        color = AccentMint,
                        fontWeight = FontWeight.Medium
                    )
                    Spacer(Modifier.height(4.dp))
                    Text(
                        "版本: ${info.releaseTag}  ·  大小: ${formatFileSize(info.sizeBytes)}",
                        fontSize = 11.sp,
                        color = TextTertiary,
                        fontFamily = FontFamily.Monospace
                    )

                    Spacer(Modifier.height(14.dp))

                    // 下载状态区
                    when (val dl = moduleDownloadState) {
                        is DownloadState.Idle -> {
                            Button(
                                onClick = onDownload,
                                modifier = Modifier.fillMaxWidth().height(44.dp),
                                shape = RoundedCornerShape(10.dp),
                                colors = ButtonDefaults.buttonColors(containerColor = AccentBlue)
                            ) {
                                Icon(Icons.Default.Download, null, Modifier.size(18.dp), tint = Color.White)
                                Spacer(Modifier.width(8.dp))
                                Text("下载模块 zip", color = Color.White, fontSize = 13.sp, fontWeight = FontWeight.SemiBold)
                            }
                            Spacer(Modifier.height(8.dp))
                            OutlinedButton(
                                onClick = onOpenInBrowser,
                                modifier = Modifier.fillMaxWidth().height(38.dp),
                                shape = RoundedCornerShape(10.dp)
                            ) {
                                Text("在浏览器中手动下载", fontSize = 11.sp)
                            }
                        }
                        is DownloadState.Downloading -> {
                            @Suppress("DEPRECATION")
                            LinearProgressIndicator(
                                progress = dl.progress / 100f,
                                modifier = Modifier.fillMaxWidth().height(8.dp),
                                color = AccentBlue,
                                trackColor = if (isDark) DeepSurfaceVariant else Color(0xFFE2E8F0)
                            )
                            Spacer(Modifier.height(6.dp))
                            Row(
                                modifier = Modifier.fillMaxWidth(),
                                horizontalArrangement = Arrangement.SpaceBetween,
                                verticalAlignment = Alignment.CenterVertically
                            ) {
                                Text(
                                    "${dl.progress}%  ·  ${formatFileSize(dl.downloadedBytes)} / ${formatFileSize(dl.totalBytes)}",
                                    fontSize = 11.sp,
                                    color = TextSecondary,
                                    fontFamily = FontFamily.Monospace
                                )
                                OutlinedButton(
                                    onClick = onCancel,
                                    modifier = Modifier.height(34.dp),
                                    shape = RoundedCornerShape(8.dp),
                                    contentPadding = PaddingValues(horizontal = 12.dp, vertical = 0.dp)
                                ) {
                                    Text("取消", fontSize = 11.sp, color = ErrorRed)
                                }
                            }
                        }
                        is DownloadState.Completed -> {
                            Button(
                                onClick = onInstall,
                                modifier = Modifier.fillMaxWidth().height(44.dp),
                                shape = RoundedCornerShape(10.dp),
                                colors = ButtonDefaults.buttonColors(containerColor = AccentMint)
                            ) {
                                Icon(Icons.Default.Extension, null, Modifier.size(18.dp), tint = Color.White)
                                Spacer(Modifier.width(8.dp))
                                Text("使用 Magisk Manager 刷入", color = Color.White, fontSize = 13.sp, fontWeight = FontWeight.SemiBold)
                            }
                            Spacer(Modifier.height(6.dp))
                            Text(
                                "模块已下载完成。点击上方按钮选择 Magisk Manager 刷入；\n" +
                                    "若无 Magisk Manager，请在文件管理器中手动导入到 Magisk。",
                                fontSize = 11.sp,
                                color = TextTertiary
                            )
                        }
                        is DownloadState.Failed -> {
                            Text("下载失败: ${dl.message}", fontSize = 12.sp, color = ErrorRed)
                            Spacer(Modifier.height(8.dp))
                            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                                Button(
                                    onClick = onDownload,
                                    modifier = Modifier.weight(1f).height(40.dp),
                                    shape = RoundedCornerShape(10.dp),
                                    colors = ButtonDefaults.buttonColors(containerColor = AccentBlue)
                                ) {
                                    Text("重试", color = Color.White, fontSize = 12.sp)
                                }
                                OutlinedButton(
                                    onClick = onOpenInBrowser,
                                    modifier = Modifier.weight(1f).height(40.dp),
                                    shape = RoundedCornerShape(10.dp)
                                ) {
                                    Text("浏览器下载", fontSize = 12.sp)
                                }
                            }
                        }
                        is DownloadState.Cancelled -> {
                            Text("下载已取消", fontSize = 12.sp, color = TextTertiary)
                            Spacer(Modifier.height(8.dp))
                            Button(
                                onClick = onDownload,
                                modifier = Modifier.fillMaxWidth().height(40.dp),
                                shape = RoundedCornerShape(10.dp),
                                colors = ButtonDefaults.buttonColors(containerColor = AccentBlue)
                            ) {
                                Text("重新下载", color = Color.White, fontSize = 12.sp)
                            }
                        }
                    }
                }
            }

            Spacer(Modifier.height(12.dp))
            Text(
                "说明：模块 zip 命名约定包含 'module' / 'magisk' / 'apex-hide' / 'hide-daemon' 关键字。\n" +
                    "下载完成后由 Magisk Manager 接管刷入流程，需已安装 Magisk。",
                fontSize = 10.sp,
                color = TextTertiary
            )
        }
    }
}

// ─── Native 库诊断卡片 ──────────────────────────

@Composable
private fun NativeLibDiagnosticCard(
    loaded: Boolean,
    failed: Boolean,
    retrying: Boolean,
    onRetry: () -> Unit,
    isDark: Boolean
) {
    val statusColor = when {
        loaded -> AccentMint
        failed -> ErrorRed
        else -> AccentGold
    }
    val statusText = when {
        loaded -> "已加载"
        failed -> "加载失败"
        else -> "未加载"
    }
    val statusIcon = when {
        loaded -> Icons.Default.CheckCircle
        failed -> Icons.Default.ErrorOutline
        else -> Icons.Default.Pending
    }

    GlassCard(cornerRadius = 16.dp, accentLine = statusColor) {
        Column(modifier = Modifier.padding(20.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Icon(
                    statusIcon,
                    contentDescription = null,
                    tint = statusColor,
                    modifier = Modifier.size(28.dp)
                )
                Spacer(Modifier.width(12.dp))
                Column(modifier = Modifier.weight(1f)) {
                    Text(
                        "libapex_root.so",
                        fontSize = 16.sp,
                        fontWeight = FontWeight.Bold,
                        color = if (isDark) TextPrimary else Color(0xFF1A1A2E)
                    )
                    Text(
                        "原生检测引擎状态: $statusText",
                        fontSize = 11.sp,
                        color = TextSecondary
                    )
                }
            }

            Spacer(Modifier.height(14.dp))

            when {
                loaded -> {
                    Text(
                        "✅ 原生库已成功加载，所有 16 层检测功能可用。",
                        fontSize = 12.sp,
                        color = AccentMint
                    )
                }
                failed -> {
                    Text(
                        "❌ 原生库加载失败。可能原因：\n" +
                            "  · 设备非 ARM64 架构（libapex_root.so 仅编译 arm64-v8a）\n" +
                            "  · SELinux 临时拒绝\n" +
                            "  · APK 安装不完整或被破坏\n\n" +
                            "点击「重试加载」可尝试重新加载（无需重启应用）。",
                        fontSize = 11.sp,
                        color = ErrorRed
                    )
                    Spacer(Modifier.height(10.dp))
                    Button(
                        onClick = onRetry,
                        enabled = !retrying,
                        modifier = Modifier.fillMaxWidth().height(44.dp),
                        shape = RoundedCornerShape(10.dp),
                        colors = ButtonDefaults.buttonColors(containerColor = AccentPurple)
                    ) {
                        if (retrying) {
                            CircularProgressIndicator(
                                modifier = Modifier.size(18.dp),
                                color = Color.White,
                                strokeWidth = 2.dp
                            )
                            Spacer(Modifier.width(8.dp))
                            Text("正在重试...", color = Color.White, fontSize = 13.sp)
                        } else {
                            Icon(Icons.Default.Refresh, null, Modifier.size(18.dp), tint = Color.White)
                            Spacer(Modifier.width(8.dp))
                            Text("重试加载", color = Color.White, fontSize = 13.sp, fontWeight = FontWeight.SemiBold)
                        }
                    }
                }
                else -> {
                    Text(
                        "⏳ 原生库尚未加载。首次扫描时会自动加载，或点击下方按钮立即加载。",
                        fontSize = 11.sp,
                        color = AccentGold
                    )
                    Spacer(Modifier.height(10.dp))
                    OutlinedButton(
                        onClick = onRetry,
                        enabled = !retrying,
                        modifier = Modifier.fillMaxWidth().height(44.dp),
                        shape = RoundedCornerShape(10.dp)
                    ) {
                        if (retrying) {
                            CircularProgressIndicator(
                                modifier = Modifier.size(18.dp),
                                strokeWidth = 2.dp
                            )
                            Spacer(Modifier.width(8.dp))
                            Text("正在加载...", fontSize = 13.sp)
                        } else {
                            Icon(Icons.Default.Download, null, Modifier.size(18.dp))
                            Spacer(Modifier.width(8.dp))
                            Text("立即加载", fontSize = 13.sp)
                        }
                    }
                }
            }

            Spacer(Modifier.height(8.dp))
            Text(
                "诊断信息：\n" +
                    "  · 库名: ${NativeLibraryLoader.LIB_NAME}\n" +
                    "  · 已加载: $loaded\n" +
                    "  · 加载失败标志: $failed",
                fontSize = 10.sp,
                color = TextTertiary,
                fontFamily = FontFamily.Monospace
            )
        }
    }
}
