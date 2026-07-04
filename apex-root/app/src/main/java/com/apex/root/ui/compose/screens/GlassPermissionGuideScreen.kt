package com.apex.root.ui.compose.screens

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Environment
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
import androidx.compose.material.icons.filled.Folder
import androidx.compose.material.icons.filled.Notifications
import androidx.compose.material.icons.filled.Security
import androidx.compose.material.icons.filled.Warning
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
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlinx.coroutines.withTimeoutOrNull
import java.util.concurrent.TimeUnit

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

    // ─── 通知权限状态 ───
    fun isNotifGranted(): Boolean = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
        ContextCompat.checkSelfPermission(context, Manifest.permission.POST_NOTIFICATIONS) ==
            PackageManager.PERMISSION_GRANTED
    } else true

    var notifGranted by remember { mutableStateOf(isNotifGranted()) }
    // 标记是否已经弹出过系统权限对话框（避免重复弹出）
    var notifRequested by remember { mutableStateOf(false) }

    // ─── 存储权限状态（MANAGE_EXTERNAL_STORAGE，Android 11+ 特殊权限）───
    fun isStorageGranted(): Boolean = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
        Environment.isExternalStorageManager()
    } else {
        ContextCompat.checkSelfPermission(context, Manifest.permission.READ_EXTERNAL_STORAGE) ==
            PackageManager.PERMISSION_GRANTED
    }

    var storageGranted by remember { mutableStateOf(isStorageGranted()) }

    // 运行时权限请求 Launcher（通知权限 — 直接弹出系统对话框）
    val notifPermissionLauncher = rememberLauncherForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { granted ->
        notifGranted = granted || isNotifGranted()
        notifRequested = true
    }

    // 多权限请求 Launcher（用于同时请求通知 + 旧版存储权限，Android 10 及以下）
    val multiPermissionLauncher = rememberLauncherForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { results ->
        results.forEach { (perm, granted) ->
            when (perm) {
                Manifest.permission.POST_NOTIFICATIONS -> {
                    notifGranted = granted || isNotifGranted()
                }
                Manifest.permission.READ_EXTERNAL_STORAGE -> {
                    storageGranted = granted || isStorageGranted()
                }
            }
        }
        notifRequested = true
    }

    // ─── 首次启动：自动弹出系统权限对话框 ───
    // 修复：原实现仅在 !notifGranted 时请求，且与 MainActivity 的请求冲突。
    // 现在改为：进入引导页后短暂延迟（等 Composable 完成组合），然后可靠地弹出
    // 系统权限对话框。使用 RequestMultiplePermissions 同时请求通知 + 存储（Android 10-）。
    LaunchedEffect(Unit) {
        // 短暂延迟，确保 Composable 已完全组合，避免与 SplashScreen 动画冲突
        delay(300)

        // 构建需要请求的权限列表
        val permsToRequest = mutableListOf<String>()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU && !notifGranted) {
            permsToRequest.add(Manifest.permission.POST_NOTIFICATIONS)
        }
        // Android 10 及以下：请求 READ_EXTERNAL_STORAGE
        if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.Q && !storageGranted) {
            permsToRequest.add(Manifest.permission.READ_EXTERNAL_STORAGE)
        }

        if (permsToRequest.isNotEmpty()) {
            try {
                multiPermissionLauncher.launch(permsToRequest.toTypedArray())
            } catch (e: Throwable) {
                // 某些设备上 launch 可能抛异常（如 Activity 已 destroy），降级为单权限请求
                try {
                    if (permsToRequest.contains(Manifest.permission.POST_NOTIFICATIONS)) {
                        notifPermissionLauncher.launch(Manifest.permission.POST_NOTIFICATIONS)
                    }
                } catch (_: Throwable) {}
            }
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
                "16 层全量 Root 检测系统",
                color = subColor,
                fontSize = 13.sp,
                textAlign = TextAlign.Center
            )
            Spacer(Modifier.height(6.dp))
            Text(
                "通过原生引擎（JNI + eBPF + 内核探针）实现硬件级真实性校验，" +
                    "检测 Magisk / KernelSU / APatch / Shamiko / ZygiskNext 等 root 框架。" +
                    "本应用仅用于检测，不会修改系统文件。",
                color = subColor,
                fontSize = 12.sp,
                textAlign = TextAlign.Center
            )

            Spacer(Modifier.height(24.dp))

            // ─── 权限提示横幅 ───
            // 首次进入必须授权的提示
            if (!notifGranted || (!storageGranted && Build.VERSION.SDK_INT <= Build.VERSION_CODES.Q)) {
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .background(
                            AccentGold.copy(alpha = if (isDark) 0.12f else 0.08f),
                            RoundedCornerShape(12.dp)
                        )
                        .padding(12.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Icon(
                        Icons.Filled.Warning,
                        contentDescription = null,
                        tint = AccentGold,
                        modifier = Modifier.size(20.dp)
                    )
                    Spacer(Modifier.width(8.dp))
                    Text(
                        "请授权以下权限以确保应用正常工作",
                        fontSize = 12.sp,
                        color = AccentGold,
                        fontWeight = FontWeight.Medium
                    )
                }
                Spacer(Modifier.height(16.dp))
            }

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

            // 通知权限卡片
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
                        try {
                            notifPermissionLauncher.launch(Manifest.permission.POST_NOTIFICATIONS)
                        } catch (e: Throwable) {
                            // 如果已被永久拒绝（"不再询问"），跳转到设置页
                            val intent = Intent(Settings.ACTION_APP_NOTIFICATION_SETTINGS).apply {
                                putExtra(Settings.EXTRA_APP_PACKAGE, context.packageName)
                                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                            }
                            runCatching { context.startActivity(intent) }
                        }
                    } else {
                        // 低版本：跳转应用通知设置页
                        val intent = Intent(Settings.ACTION_APP_NOTIFICATION_SETTINGS).apply {
                            putExtra(Settings.EXTRA_APP_PACKAGE, context.packageName)
                            addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                        }
                        runCatching { context.startActivity(intent) }
                    }
                }
            )

            Spacer(Modifier.height(14.dp))

            // 存储权限卡片（用于导出报告）
            PermissionRow(
                icon = Icons.Filled.Folder,
                iconTint = if (storageGranted) AccentMint else AccentGold,
                title = "存储权限",
                detail = if (storageGranted) {
                    "已授权（可导出报告到任意目录）"
                } else {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                        "未授权（需在设置中开启「所有文件访问权限」）"
                    } else {
                        "未授权（用于导出检测报告）"
                    }
                },
                actionLabel = if (storageGranted) null else "去设置",
                onAction = {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                        // Android 11+：MANAGE_EXTERNAL_STORAGE 是特殊权限，只能跳设置页
                        val intent = Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION).apply {
                            data = Uri.parse("package:${context.packageName}")
                            addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                        }
                        runCatching { context.startActivity(intent) }
                    } else {
                        // Android 10 及以下：运行时请求
                        try {
                            multiPermissionLauncher.launch(arrayOf(Manifest.permission.READ_EXTERNAL_STORAGE))
                        } catch (e: Throwable) {
                            val intent = Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS).apply {
                                data = Uri.parse("package:${context.packageName}")
                                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                            }
                            runCatching { context.startActivity(intent) }
                        }
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

            Spacer(Modifier.height(8.dp))
            Text(
                "点击「进入应用」表示您已了解所需权限并同意继续",
                color = subColor,
                fontSize = 10.sp,
                textAlign = TextAlign.Center
            )
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
 * 修复：使用 waitFor(timeout) + destroyForcibly()，避免 IO 线程被阻塞。
 * - 设备未安装 su：exec 抛 IOException → 返回"未安装"
 * - su 已安装但需用户授权：waitFor 阻塞 → 超时后返回"超时"
 * - su 已授权：返回 uid=0
 */
private suspend fun checkRootStatus(): Pair<Boolean, String> {
    var process: Process? = null
    try {
        return withTimeoutOrNull(3000L) {
            runCatching {
                process = Runtime.getRuntime().exec(arrayOf("su", "-c", "id"))
                val p = process!!
                val text = p.inputStream.bufferedReader().readText()
                val exited = p.waitFor(3000, TimeUnit.MILLISECONDS)
                val exit = if (exited) p.exitValue() else -1
                if (exit == 0 && text.contains("uid=0")) {
                    true to "已获取（uid=0）"
                } else {
                    false to "未授权（exit=$exit）"
                }
            }.getOrElse { e ->
                false to "未安装 root 框架（${e.message ?: e.javaClass.simpleName}）"
            }
        } ?: (false to "检测超时（su 未响应，可能需用户授权）")
    } finally {
        try {
            process?.destroyForcibly()
        } catch (_: Throwable) {}
    }
}
