package com.apex.root.ui.compose.screens

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.provider.Settings
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.CheckCircle
import androidx.compose.material.icons.filled.Cancel
import androidx.compose.material.icons.filled.Notifications
import androidx.compose.material.icons.filled.Security
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.content.ContextCompat
import com.apex.root.ui.compose.*
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlinx.coroutines.withTimeoutOrNull

private enum class RootStatus { UNKNOWN, AVAILABLE, UNAVAILABLE }

@Composable
fun GlassPermissionGuideScreen(onFinished: () -> Unit) {
    val isDark = LocalIsDarkTheme.current
    val bg = if (isDark) DeepBackground else Color(0xFFF1F5F9)
    val hColor = if (isDark) TextPrimary else Color(0xFF0F172A)
    val subColor = if (isDark) TextSecondary else Color(0xFF475569)
    val context = LocalContext.current
    val scope = rememberCoroutineScope()

    var rootStatus by remember { mutableStateOf(RootStatus.UNKNOWN) }
    var rootDetail by remember { mutableStateOf("正在检测 ROOT 状态...") }

    // 通知权限实时状态
    fun isNotifGranted(): Boolean = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
        ContextCompat.checkSelfPermission(context, Manifest.permission.POST_NOTIFICATIONS) ==
            PackageManager.PERMISSION_GRANTED
    } else true

    var notifGranted by remember { mutableStateOf(isNotifGranted()) }

    // 运行时权限请求 Launcher —— 直接在应用内弹出系统授权对话框
    val notifPermissionLauncher = rememberLauncherForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { granted ->
        notifGranted = granted || isNotifGranted()
    }

    // 启动时自动请求一次通知权限（Android 13+），并并行检测 Root
    LaunchedEffect(Unit) {
        // 自动请求通知权限（仅 Android 13+ 且未授权时）
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU && !notifGranted) {
            notifPermissionLauncher.launch(Manifest.permission.POST_NOTIFICATIONS)
        }
        // 并行检测 Root 状态（带 3 秒超时，避免 su 弹窗导致 ANR）
        scope.launch {
            val (ok, detail) = withContext(Dispatchers.IO) { checkRootStatus() }
            rootStatus = if (ok) RootStatus.AVAILABLE else RootStatus.UNAVAILABLE
            rootDetail = detail
        }
    }

    Box(modifier = Modifier.fillMaxSize().background(bg), contentAlignment = Alignment.Center) {
        Column(
            modifier = Modifier
                .padding(24.dp)
                .fillMaxWidth()
                .verticalScroll(rememberScrollState())
                .pristineGlass(cornerRadius = 32.dp)
                .padding(32.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            // Logo
            Box(
                modifier = Modifier
                    .size(96.dp)
                    .background(
                        if (isDark) AccentPurple else Color(0xFF7C5CFC),
                        CircleShape
                    ),
                contentAlignment = Alignment.Center
            ) {
                Icon(
                    imageVector = Icons.Filled.Security,
                    contentDescription = null,
                    tint = Color.White,
                    modifier = Modifier.size(56.dp)
                )
            }

            Spacer(Modifier.height(20.dp))
            Text(
                "APEX Root",
                fontSize = 26.sp,
                fontWeight = FontWeight.Bold,
                color = hColor,
                textAlign = TextAlign.Center
            )
            Spacer(Modifier.height(8.dp))
            Text(
                "16 层全量 Root 检测与防御系统",
                color = subColor,
                fontSize = 13.sp,
                textAlign = TextAlign.Center
            )
            Spacer(Modifier.height(6.dp))
            Text(
                "通过原生引擎（JNI + eBPF + 内核探针）实现硬件级真实性校验，" +
                    "检测 Magisk / KernelSU / APatch / Shamiko / ZygiskNext 等 root 框架，" +
                    "并提供自保护、沙箱隔离与 HWID 伪装能力。",
                color = subColor,
                fontSize = 12.sp,
                textAlign = TextAlign.Center
            )

            Spacer(Modifier.height(28.dp))

            // ROOT 状态卡片
            PermissionRow(
                icon = if (rootStatus == RootStatus.AVAILABLE) Icons.Filled.CheckCircle
                       else Icons.Filled.Cancel,
                iconTint = if (rootStatus == RootStatus.AVAILABLE) AccentMint
                           else if (rootStatus == RootStatus.UNAVAILABLE) ErrorRed
                           else AccentGold,
                title = "ROOT 权限",
                detail = rootDetail,
                actionLabel = if (rootStatus == RootStatus.UNAVAILABLE) "获取 Magisk" else null,
                onAction = {
                    val intent = Intent(Intent.ACTION_VIEW, Uri.parse("https://github.com/topjohnwu/Magisk")).apply {
                        addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                    }
                    runCatching { context.startActivity(intent) }
                }
            )

            Spacer(Modifier.height(14.dp))

            // 通知权限卡片 —— 直接在应用内请求系统授权
            PermissionRow(
                icon = Icons.Filled.Notifications,
                iconTint = if (notifGranted) AccentMint else AccentGold,
                title = "通知权限",
                detail = if (notifGranted) "已授权" else "未授权（用于推送扫描结果与告警）",
                actionLabel = when {
                    notifGranted -> null
                    Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU -> "去授权"
                    else -> "去设置"
                },
                onAction = {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU && !notifGranted) {
                        // 直接弹出系统运行时权限对话框
                        notifPermissionLauncher.launch(Manifest.permission.POST_NOTIFICATIONS)
                    } else {
                        // 低版本或已被永久拒绝：跳转应用通知设置页
                        val intent = Intent(Settings.ACTION_APP_NOTIFICATION_SETTINGS).apply {
                            putExtra(Settings.EXTRA_APP_PACKAGE, context.packageName)
                            addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                        }
                        runCatching { context.startActivity(intent) }
                    }
                }
            )

            Spacer(Modifier.height(32.dp))

            Button(
                onClick = onFinished,
                modifier = Modifier.fillMaxWidth().height(52.dp),
                shape = RoundedCornerShape(16.dp),
                colors = ButtonDefaults.buttonColors(
                    containerColor = if (isDark) AccentPurple else Color(0xFF7C5CFC)
                )
            ) {
                Text("进入应用", color = Color.White, fontSize = 16.sp, fontWeight = FontWeight.SemiBold)
            }
        }
    }
}

@Composable
private fun PermissionRow(
    icon: androidx.compose.ui.graphics.vector.ImageVector,
    iconTint: Color,
    title: String,
    detail: String,
    actionLabel: String?,
    onAction: () -> Unit
) {
    val isDark = LocalIsDarkTheme.current
    val hColor = if (isDark) TextPrimary else Color(0xFF0F172A)
    val subColor = if (isDark) TextSecondary else Color(0xFF475569)
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .background(
                if (isDark) DeepSurfaceVariant else Color(0xFFE2E8F0),
                RoundedCornerShape(16.dp)
            )
            .padding(16.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Icon(icon, contentDescription = null, tint = iconTint, modifier = Modifier.size(28.dp))
        Spacer(Modifier.width(16.dp))
        Column(modifier = Modifier.weight(1f)) {
            Text(title, color = hColor, fontWeight = FontWeight.SemiBold, fontSize = 15.sp)
            Text(detail, color = subColor, fontSize = 12.sp)
        }
        if (actionLabel != null) {
            Text(
                actionLabel,
                color = AccentPurple,
                fontSize = 13.sp,
                fontWeight = FontWeight.SemiBold,
                modifier = Modifier.clickable { onAction() }
            )
        }
    }
}

/**
 * 检测 Root 状态。带 3 秒超时，避免 su 弹窗阻塞导致 ANR/卡死。
 * - 设备未安装 su：exec 抛 IOException → 返回"未安装"
 * - su 已安装但需用户授权：waitFor 阻塞 → 超时后返回"超时"
 * - su 已授权：返回 uid=0
 */
private suspend fun checkRootStatus(): Pair<Boolean, String> =
    withTimeoutOrNull(3000L) {
        runCatching {
            val process = Runtime.getRuntime().exec(arrayOf("su", "-c", "id"))
            val text = process.inputStream.bufferedReader().readText()
            val exit = process.waitFor()
            if (exit == 0 && text.contains("uid=0")) {
                true to "已获取（uid=0）"
            } else {
                false to "未授权（exit=$exit）"
            }
        }.getOrElse { e ->
            false to "未安装 root 框架（${e.message ?: e.javaClass.simpleName}）"
        }
    } ?: (false to "检测超时（su 未响应，可能需用户授权）")
