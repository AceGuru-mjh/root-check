package com.apex.root.ui.compose.screens

import android.content.Intent
import android.net.Uri
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
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
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.apex.root.ui.compose.*
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * 关于 / 帮助屏幕 — 精致版
 *
 * 分区清晰：应用信息 → 开发者联系方式 → 文档 → 核心功能 → 隐私声明 → 开源许可 → 免责声明
 * 每个可点击项（邮箱 / GitHub / 文档链接）均带 try-catch 防止 ActivityNotFoundException 闪退。
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun AboutScreen(
    onBack: () -> Unit
) {
    val context = LocalContext.current
    val isDark = LocalIsDarkTheme.current
    val scrollBehavior = TopAppBarDefaults.exitUntilCollapsedScrollBehavior()

    // 读取应用版本信息
    val packageInfo = remember {
        try {
            val pm = context.packageManager
            val info = pm.getPackageInfo(context.packageName, 0)
            Triple(
                info.versionName ?: "未知",
                if (android.os.Build.VERSION.SDK_INT >= 28) info.longVersionCode.toString() else info.versionCode.toString(),
                SimpleDateFormat("yyyy-MM-dd HH:mm", Locale.getDefault()).format(Date(info.firstInstallTime))
            )
        } catch (e: Throwable) {
            Triple("未知", "0", "未知")
        }
    }
    val (versionName, versionCode, installTime) = packageInfo

    // 安全打开外部链接
    fun openUrl(url: String) {
        runCatching {
            val intent = Intent(Intent.ACTION_VIEW, Uri.parse(url)).apply {
                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            }
            context.startActivity(intent)
        }
    }
    fun openEmail(email: String) {
        runCatching {
            val intent = Intent(Intent.ACTION_SENDTO, Uri.parse("mailto:$email")).apply {
                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            }
            context.startActivity(intent)
        }
    }

    Scaffold(
        modifier = Modifier.nestedScroll(scrollBehavior.nestedScrollConnection),
        topBar = {
            CollapsibleGlassTopBar(
                title = "关于",
                collapsedFraction = scrollBehavior.state.collapsedFraction,
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.Default.ArrowBack, "返回")
                    }
                }
            )
        },
        containerColor = Color.Transparent
    ) { padding ->
        LiquidGlassContainer(
            fluidColorsDark = PageFluidColors.settings,
            fluidColorsLight = PageFluidColors.settingsLight
        ) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(padding)
                    .verticalScroll(rememberScrollState())
                    .padding(horizontal = 20.dp, vertical = 16.dp),
                verticalArrangement = Arrangement.spacedBy(16.dp)
            ) {
                // ── 应用信息卡片（顶部 hero）──
                GlassCard(cornerRadius = 24.dp, accentLine = AccentPurple) {
                    Column(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalAlignment = Alignment.CenterHorizontally
                    ) {
                        Box(
                            modifier = Modifier
                                .size(84.dp)
                                .clip(RoundedCornerShape(22.dp))
                                .background(
                                    Brush.linearGradient(
                                        listOf(AccentPurple.copy(alpha = 0.25f), AccentBlue.copy(alpha = 0.12f))
                                    )
                                ),
                            contentAlignment = Alignment.Center
                        ) {
                            Icon(
                                Icons.Default.Security,
                                contentDescription = null,
                                tint = AccentPurple,
                                modifier = Modifier.size(44.dp)
                            )
                        }
                        Spacer(Modifier.height(14.dp))
                        Text(
                            "APEX-Root",
                            fontSize = 24.sp,
                            fontWeight = FontWeight.Bold,
                            color = TextPrimary
                        )
                        Text(
                            "全能安全平台 · 16 层 Root 检测与防御",
                            fontSize = 12.sp,
                            color = TextSecondary
                        )
                        Spacer(Modifier.height(10.dp))
                        Row(
                            horizontalArrangement = Arrangement.spacedBy(8.dp),
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            InfoChip("v$versionName")
                            InfoChip("build #$versionCode")
                            InfoChip(android.os.Build.SUPPORTED_ABIS.firstOrNull() ?: "?")
                        }
                    }
                }

                // ── 开发者联系方式 ──
                SectionTitle("开发者联系方式")
                GlassCard(cornerRadius = 18.dp, accentLine = AccentBlue) {
                    Column(verticalArrangement = Arrangement.spacedBy(2.dp)) {
                        ContactItem(
                            icon = Icons.Default.Person,
                            iconTint = AccentPurple,
                            label = "开发者",
                            value = "MJH (mengjinghao)"
                        )
                        ContactDivider()
                        ContactItem(
                            icon = Icons.Default.Email,
                            iconTint = AccentBlue,
                            label = "邮箱",
                            value = "mjh4117222@gmail.com",
                            clickable = true,
                            onClick = { openEmail("mjh4117222@gmail.com") }
                        )
                        ContactDivider()
                        ContactItem(
                            icon = Icons.Default.Code,
                            iconTint = AccentMint,
                            label = "GitHub",
                            value = "github.com/mengjinghao",
                            clickable = true,
                            onClick = { openUrl("https://github.com/mengjinghao") }
                        )
                        ContactDivider()
                        ContactItem(
                            icon = Icons.Default.BugReport,
                            iconTint = AccentGold,
                            label = "问题反馈",
                            value = "提交 Issue / 反馈 Bug",
                            clickable = true,
                            onClick = { openUrl("https://github.com/mengjinghao/root-check/issues") }
                        )
                    }
                }

                // ── 文档 ──
                SectionTitle("文档")
                GlassCard(cornerRadius = 18.dp, accentLine = AccentMint) {
                    Column(verticalArrangement = Arrangement.spacedBy(2.dp)) {
                        DocItem(
                            icon = Icons.Default.MenuBook,
                            title = "使用文档",
                            subtitle = "快速上手 · 功能说明 · FAQ",
                            onClick = { openUrl("https://github.com/mengjinghao/root-check#readme") }
                        )
                        ContactDivider()
                        DocItem(
                            icon = Icons.Default.Architecture,
                            title = "技术架构",
                            subtitle = "16 层检测引擎 · eBPF 防火墙 · 命名空间隔离",
                            onClick = { openUrl("https://github.com/mengjinghao/root-check/blob/main/apex-root/CODE_REVIEW.md") }
                        )
                        ContactDivider()
                        DocItem(
                            icon = Icons.Default.Security,
                            title = "安全评估报告",
                            subtitle = "eBPF 代码审查 · SELinux 策略 · 隐藏机制",
                            onClick = { openUrl("https://github.com/mengjinghao/root-check/blob/main/apex-root/CODE_REVIEW.md") }
                        )
                        ContactDivider()
                        DocItem(
                            icon = Icons.Default.Download,
                            title = "下载最新版",
                            subtitle = "多架构 APK · Release 发布",
                            onClick = { openUrl("https://github.com/mengjinghao/root-check/releases") }
                        )
                    }
                }

                // ── 设备信息 ──
                SectionTitle("设备信息")
                GlassCard(cornerRadius = 18.dp) {
                    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                        InfoRow("应用版本", "v$versionName (build #$versionCode)")
                        InfoRow("安装时间", installTime)
                        InfoRow("Android 版本", "${android.os.Build.VERSION.RELEASE} (API ${android.os.Build.VERSION.SDK_INT})")
                        InfoRow("设备型号", "${android.os.Build.MANUFACTURER} ${android.os.Build.MODEL}")
                        InfoRow("支持架构", android.os.Build.SUPPORTED_ABIS.joinToString(", "))
                    }
                }

                // ── 核心功能 ──
                SectionTitle("核心功能")
                GlassCard(cornerRadius = 18.dp) {
                    Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
                        FeatureItem(
                            icon = Icons.Default.Search,
                            title = "16 层检测",
                            description = "覆盖 Magisk / KernelSU / APatch / ZygiskNext / LSPosed / Frida 等主流 root 方案与隐藏框架。"
                        )
                        FeatureItem(
                            icon = Icons.Default.VisibilityOff,
                            title = "隐藏模式",
                            description = "Ring3 root 级隐藏，Detection / Hide / Game 三模式。eBPF 防火墙 (Android 12+) + namespace 回退。"
                        )
                        FeatureItem(
                            icon = Icons.Default.Healing,
                            title = "治愈系统",
                            description = "4 级修复：轻度处理 / 标准修复 / 深度恢复 / 完全重置。"
                        )
                        FeatureItem(
                            icon = Icons.Default.Shield,
                            title = "实时守护",
                            description = "后台监控系统完整性，检测到篡改即时告警。"
                        )
                        FeatureItem(
                            icon = Icons.Default.Fingerprint,
                            title = "硬件伪装",
                            description = "HWID / 序列号 / build fingerprint 伪装，防止设备指纹追踪。"
                        )
                    }
                }

                // ── 隐私声明 ──
                SectionTitle("隐私声明")
                GlassCard(cornerRadius = 18.dp) {
                    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                        Text(
                            "APEX-Root 完全本地运行，不上传任何设备信息到云端。",
                            fontSize = 13.sp,
                            color = TextPrimary,
                            fontWeight = FontWeight.Medium
                        )
                        Text(
                            "• 检测结果仅存储在设备本地\n• 报告导出由用户主动触发\n• 后量子签名用于报告防篡改\n• 不收集使用统计或崩溃日志",
                            fontSize = 12.sp,
                            color = TextTertiary,
                            lineHeight = 18.sp
                        )
                    }
                }

                // ── 开源许可 ──
                SectionTitle("开源许可")
                GlassCard(cornerRadius = 18.dp) {
                    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                        LicenseItem("Jetpack Compose", "Apache 2.0")
                        LicenseItem("Material 3 Components", "Apache 2.0")
                        LicenseItem("liboqs (后量子签名)", "MIT")
                        LicenseItem("AndroidX", "Apache 2.0")
                        LicenseItem("Kotlin Coroutines", "Apache 2.0")
                    }
                }

                // ── 免责声明 ──
                SectionTitle("免责声明")
                GlassCard(cornerRadius = 18.dp, accentLine = ErrorRed) {
                    Text(
                        "本应用仅供安全研究与设备完整性评估使用。" +
                            "使用本应用进行的任何操作（root 隐藏、系统治愈、硬件伪装）由用户自行承担风险。" +
                            "开发者不对因使用本应用导致的设备损坏、数据丢失负责。请遵守当地法律法规。",
                        fontSize = 12.sp,
                        color = TextTertiary,
                        lineHeight = 18.sp
                    )
                }

                Spacer(Modifier.height(24.dp))

                // ── 底部签名 ──
                Text(
                    "APEX-Root v$versionName · build #$versionCode\nMade with ❤ by MJH",
                    fontSize = 11.sp,
                    color = TextTertiary,
                    fontFamily = FontFamily.Monospace,
                    modifier = Modifier.fillMaxWidth(),
                    textAlign = TextAlign.Center,
                    lineHeight = 16.sp
                )

                Spacer(Modifier.height(16.dp))
            }
        }
    }
}

// ─── 私有组件 ──────────────────────────────────────────────

@Composable
private fun SectionTitle(title: String) {
    Text(
        title,
        fontSize = 14.sp,
        fontWeight = FontWeight.SemiBold,
        color = TextSecondary,
        modifier = Modifier.padding(top = 8.dp, start = 4.dp)
    )
}

@Composable
private fun InfoChip(text: String) {
    Box(
        modifier = Modifier
            .clip(RoundedCornerShape(8.dp))
            .background(AccentPurple.copy(alpha = 0.1f))
            .padding(horizontal = 9.dp, vertical = 4.dp)
    ) {
        Text(text, fontSize = 10.sp, color = AccentPurple, fontWeight = FontWeight.Medium)
    }
}

@Composable
private fun ContactItem(
    icon: ImageVector,
    iconTint: Color,
    label: String,
    value: String,
    clickable: Boolean = false,
    onClick: () -> Unit = {}
) {
    val rowModifier = if (clickable) {
        Modifier.fillMaxWidth().clickable { onClick() }.padding(vertical = 10.dp)
    } else {
        Modifier.fillMaxWidth().padding(vertical = 10.dp)
    }
    Row(
        modifier = rowModifier,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Box(
            modifier = Modifier
                .size(34.dp)
                .clip(RoundedCornerShape(10.dp))
                .background(iconTint.copy(alpha = 0.14f)),
            contentAlignment = Alignment.Center
        ) {
            Icon(icon, contentDescription = null, tint = iconTint, modifier = Modifier.size(18.dp))
        }
        Spacer(Modifier.width(12.dp))
        Column(modifier = Modifier.weight(1f)) {
            Text(label, fontSize = 11.sp, color = TextTertiary)
            Text(
                value,
                fontSize = 13.sp,
                color = if (clickable) iconTint else TextPrimary,
                fontWeight = FontWeight.Medium,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis
            )
        }
        if (clickable) {
            Icon(
                Icons.Default.ChevronRight,
                contentDescription = null,
                tint = TextTertiary,
                modifier = Modifier.size(18.dp)
            )
        }
    }
}

@Composable
private fun DocItem(
    icon: ImageVector,
    title: String,
    subtitle: String,
    onClick: () -> Unit
) {
    Row(
        modifier = Modifier.fillMaxWidth().clickable { onClick() }.padding(vertical = 10.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Box(
            modifier = Modifier
                .size(34.dp)
                .clip(RoundedCornerShape(10.dp))
                .background(AccentMint.copy(alpha = 0.14f)),
            contentAlignment = Alignment.Center
        ) {
            Icon(icon, contentDescription = null, tint = AccentMint, modifier = Modifier.size(18.dp))
        }
        Spacer(Modifier.width(12.dp))
        Column(modifier = Modifier.weight(1f)) {
            Text(title, fontSize = 13.sp, color = TextPrimary, fontWeight = FontWeight.SemiBold)
            Text(
                subtitle,
                fontSize = 11.sp,
                color = TextTertiary,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis
            )
        }
        Icon(
            Icons.Default.ChevronRight,
            contentDescription = null,
            tint = TextTertiary,
            modifier = Modifier.size(18.dp)
        )
    }
}

@Composable
private fun ContactDivider() {
    // material3 1.1.2 无 HorizontalDivider，用 Box 模拟
    Box(
        modifier = Modifier
            .padding(start = 46.dp)
            .fillMaxWidth()
            .height(0.5.dp)
            .background(
                if (LocalIsDarkTheme.current) Color.White.copy(alpha = 0.06f) else Color.Black.copy(alpha = 0.04f)
            )
    )
}

@Composable
private fun InfoRow(label: String, value: String) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(label, fontSize = 12.sp, color = TextTertiary)
        Text(
            value,
            fontSize = 12.sp,
            color = TextPrimary,
            fontWeight = FontWeight.Medium,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis
        )
    }
}

@Composable
private fun FeatureItem(icon: ImageVector, title: String, description: String) {
    Row(modifier = Modifier.fillMaxWidth()) {
        Icon(
            icon,
            contentDescription = null,
            tint = AccentPurple,
            modifier = Modifier.size(20.dp)
        )
        Spacer(Modifier.width(12.dp))
        Column(modifier = Modifier.weight(1f)) {
            Text(title, fontSize = 13.sp, fontWeight = FontWeight.SemiBold, color = TextPrimary)
            Spacer(Modifier.height(2.dp))
            Text(
                description,
                fontSize = 11.sp,
                color = TextSecondary,
                lineHeight = 16.sp
            )
        }
    }
}

@Composable
private fun LicenseItem(name: String, license: String) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(name, fontSize = 12.sp, color = TextPrimary)
        Text(license, fontSize = 11.sp, color = TextTertiary)
    }
}
