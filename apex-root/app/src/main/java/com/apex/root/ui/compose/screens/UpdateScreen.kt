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
import com.apex.root.data.updater.DownloadState
import com.apex.root.ui.compose.*
import com.apex.root.viewmodel.UpdateUiState
import com.apex.root.viewmodel.UpdateViewModel
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
    val context = LocalContext.current

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
