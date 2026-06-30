package com.apex.root.ui.compose.screens

import androidx.compose.foundation.background
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
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.apex.root.ui.compose.*
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * 关于 / 帮助屏幕
 *
 * 显示应用版本、构建信息、功能介绍、隐私政策链接、开源许可等。
 * 原项目缺失此屏幕，SettingsScreen 的 AboutGroup 仅显示版本号字符串，
 * "开源许可"无 onClick。本屏幕补全这些功能。
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
                // 应用信息卡片
                GlassCard(cornerRadius = 20.dp, accentLine = AccentPurple) {
                    Column(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalAlignment = Alignment.CenterHorizontally
                    ) {
                        // 应用图标
                        Box(
                            modifier = Modifier
                                .size(80.dp)
                                .clip(RoundedCornerShape(20.dp))
                                .background(AccentPurple.copy(alpha = 0.15f)),
                            contentAlignment = Alignment.Center
                        ) {
                            Icon(
                                Icons.Default.Security,
                                contentDescription = null,
                                tint = AccentPurple,
                                modifier = Modifier.size(40.dp)
                            )
                        }
                        Spacer(Modifier.height(12.dp))
                        Text(
                            "APEX-Root",
                            fontSize = 22.sp,
                            fontWeight = FontWeight.Bold,
                            color = TextPrimary
                        )
                        Text(
                            "环境检测 · Environment Detection",
                            fontSize = 12.sp,
                            color = TextSecondary
                        )
                        Spacer(Modifier.height(8.dp))
                        Row(
                            horizontalArrangement = Arrangement.spacedBy(8.dp),
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            InfoChip("v$versionName")
                            InfoChip("build #$versionCode")
                        }
                    }
                }

                // 设备信息
                SectionTitle("设备信息")
                GlassCard(cornerRadius = 16.dp) {
                    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                        InfoRow("应用版本", "v$versionName (build #$versionCode)")
                        InfoRow("安装时间", installTime)
                        InfoRow("Android 版本", "${android.os.Build.VERSION.RELEASE} (API ${android.os.Build.VERSION.SDK_INT})")
                        InfoRow("设备型号", "${android.os.Build.MANUFACTURER} ${android.os.Build.MODEL}")
                        InfoRow("架构", android.os.Build.SUPPORTED_ABIS.firstOrNull() ?: "未知")
                    }
                }

                // 功能介绍
                SectionTitle("核心功能")
                GlassCard(cornerRadius = 16.dp) {
                    Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
                        FeatureItem(
                            icon = Icons.Default.Search,
                            title = "16 层检测",
                            description = "从系统属性到固件完整性，覆盖 Magisk / KernelSU / APatch / ZygiskNext / LSPosed / Frida 等主流 root 方案与隐藏框架。"
                        )
                        FeatureItem(
                            icon = Icons.Default.VisibilityOff,
                            title = "隐藏模式",
                            description = "Ring3 root 级隐藏，支持 Detection / Hide / Game 三模式切换。基于 eBPF 防火墙 (Android 12+) + mount namespace 回退 (Android 10-11)。"
                        )
                        FeatureItem(
                            icon = Icons.Default.Healing,
                            title = "治愈系统",
                            description = "一键清除 root 痕迹，4 级修复方案：轻度处理 / 标准修复 / 深度恢复 / 完全重置。"
                        )
                        FeatureItem(
                            icon = Icons.Default.Shield,
                            title = "实时守护",
                            description = "APEX-Guard 后台监控系统完整性，检测到篡改即时告警。"
                        )
                        FeatureItem(
                            icon = Icons.Default.Fingerprint,
                            title = "硬件伪装",
                            description = "HWID / 序列号 / build fingerprint 伪装，防止设备指纹追踪。"
                        )
                    }
                }

                // 评分算法说明
                SectionTitle("评分算法")
                GlassCard(cornerRadius = 16.dp) {
                    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                        Text(
                            "APEX-Root 的风险评分不是简单的'发现 N 项告警 = 分数 Y'。" +
                                    "评分算法基于以下核心原则：",
                            fontSize = 13.sp,
                            color = TextSecondary
                        )
                        Spacer(Modifier.height(4.dp))
                        Text(
                            "1. 跨层指数加权：同时跨多个检测层（属性 + 内存 + 内核 + 时序）的告警权重指数级高于单层告警。\n" +
                                    "2. 确定性优先：100 条'可能'级告警不如 1 条'确认'级告警。\n" +
                                    "3. 相关性增强：'可疑系统属性' + '已知 Magisk 内存指纹'在同进程出现，比两个独立告警严重得多。",
                            fontSize = 12.sp,
                            color = TextTertiary,
                            lineHeight = 18.sp
                        )
                    }
                }

                // 隐私声明
                SectionTitle("隐私声明")
                GlassCard(cornerRadius = 16.dp) {
                    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                        Text(
                            "APEX-Root 完全本地运行，不上传任何设备信息到云端。",
                            fontSize = 13.sp,
                            color = TextPrimary,
                            fontWeight = FontWeight.Medium
                        )
                        Text(
                            "• 检测结果仅存储在设备本地（/data/data/com.apex.root/）\n" +
                                    "• 报告导出由用户主动触发，不含敏感标识符\n" +
                                    "• 后量子签名（ML-DSA-65）用于报告防篡改，密钥每次生成\n" +
                                    "• 不收集、不分享任何使用统计或崩溃日志",
                            fontSize = 12.sp,
                            color = TextTertiary,
                            lineHeight = 18.sp
                        )
                    }
                }

                // 开源许可
                SectionTitle("开源许可")
                GlassCard(cornerRadius = 16.dp) {
                    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                        LicenseItem("Jetpack Compose", "Apache License 2.0")
                        LicenseItem("Material 3 Components", "Apache License 2.0")
                        LicenseItem("liboqs (后量子签名)", "MIT License")
                        LicenseItem("AndroidX", "Apache License 2.0")
                        LicenseItem("Kotlin Coroutines", "Apache License 2.0")
                    }
                }

                // 免责声明
                SectionTitle("免责声明")
                GlassCard(cornerRadius = 16.dp, accentLine = ErrorRed) {
                    Text(
                        "本应用仅供安全研究与设备完整性评估使用。" +
                                "使用本应用进行的任何操作（包括但不限于 root 隐藏、系统治愈、硬件伪装）由用户自行承担风险。" +
                                "开发者不对因使用本应用导致的任何设备损坏、数据丢失或违反服务条款负责。" +
                                "请遵守当地法律法规，不要将本应用用于欺诈或恶意目的。",
                        fontSize = 12.sp,
                        color = TextTertiary,
                        lineHeight = 18.sp
                    )
                }

                Spacer(Modifier.height(32.dp))

                // 底部版本号
                Text(
                    "APEX-Root v$versionName · build #$versionCode",
                    fontSize = 11.sp,
                    color = TextTertiary,
                    modifier = Modifier.fillMaxWidth(),
                    textAlign = androidx.compose.ui.text.style.TextAlign.Center
                )

                Spacer(Modifier.height(16.dp))
            }
        }
    }
}

@Composable
private fun SectionTitle(title: String) {
    Text(
        title,
        fontSize = 14.sp,
        fontWeight = FontWeight.SemiBold,
        color = TextSecondary,
        modifier = Modifier.padding(top = 8.dp)
    )
}

@Composable
private fun InfoChip(text: String) {
    Box(
        modifier = Modifier
            .clip(RoundedCornerShape(8.dp))
            .background(AccentPurple.copy(alpha = 0.1f))
            .padding(horizontal = 8.dp, vertical = 4.dp)
    ) {
        Text(text, fontSize = 11.sp, color = AccentPurple, fontWeight = FontWeight.Medium)
    }
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
            overflow = androidx.compose.ui.text.style.TextOverflow.Ellipsis
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
