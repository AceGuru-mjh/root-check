package com.apex.root.ui.compose

import androidx.compose.animation.core.*
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.CheckCircle
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.blur
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

private val DayAccent = Color(0xFF0D9488)
private val DayAccentSoft = Color(0xFF2DD4BF)
private val DayBg = Color(0xFFF1F5F9)

// ═══════════════════════════════════════════════════════════════════
// 1.  Radar Scanner — scanning animation + scrolling log
// ═══════════════════════════════════════════════════════════════════

@Composable
fun LiquidRadarScanner(
    scanningLogs: List<String>,
    isActive: Boolean = true
) {
    if (!isActive) return

    val transition = rememberInfiniteTransition(label = "radar")
    val wave1 by transition.animateFloat(0f, 1f, infiniteRepeatable(tween(2000, easing = LinearEasing)), label = "w1")
    val wave2 by transition.animateFloat(0f, 1f, infiniteRepeatable(tween(2000, delayMillis = 660, easing = LinearEasing)), label = "w2")
    val wave3 by transition.animateFloat(0f, 1f, infiniteRepeatable(tween(2000, delayMillis = 1320, easing = LinearEasing)), label = "w3")

    val isDark = LocalIsDarkTheme.current
    val coreColor = if (isDark) AccentMint else DayAccent
    val waveColor = if (isDark) AccentMint else DayAccentSoft
    val textColor = if (isDark) TextPrimary else TextPrimary

    Column(
        modifier = Modifier.fillMaxWidth(),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Box(modifier = Modifier.size(200.dp), contentAlignment = Alignment.Center) {
            listOf(wave1, wave2, wave3).forEach { progress ->
                Box(
                    modifier = Modifier
                        .size(200.dp)
                        .graphicsLayer {
                            scaleX = progress
                            scaleY = progress
                            alpha = 1f - progress
                        }
                        .background(
                            Brush.radialGradient(listOf(waveColor.copy(alpha = 0.3f), Color.Transparent)),
                            CircleShape
                        )
                )
            }
            Box(
                modifier = Modifier
                    .size(72.dp)
                    .background(coreColor, CircleShape),
                contentAlignment = Alignment.Center
            ) {
                Text("Scan", color = Color.White, fontWeight = FontWeight.Bold, fontSize = 13.sp)
            }
        }

        Spacer(Modifier.height(24.dp))

        Box(
            modifier = Modifier
                .fillMaxWidth()
                .height(150.dp)
                .pristineGlass(cornerRadius = 16.dp)
                .padding(14.dp)
        ) {
            val listState = rememberLazyListState()
            LaunchedEffect(scanningLogs.size) {
                if (scanningLogs.isNotEmpty()) listState.animateScrollToItem(scanningLogs.size - 1)
            }
            LazyColumn(state = listState, modifier = Modifier.fillMaxSize()) {
                items(scanningLogs) { log ->
                    Text("> $log", color = textColor, fontSize = 12.sp, modifier = Modifier.padding(vertical = 3.dp))
                }
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
// 2.  Skeleton shimmer card
// ═══════════════════════════════════════════════════════════════════

@Composable
fun GlassSkeletonCard(modifier: Modifier = Modifier) {
    val transition = rememberInfiniteTransition(label = "shimmer")
    val translate by transition.animateFloat(
        initialValue = 0f, targetValue = 1000f,
        animationSpec = infiniteRepeatable(tween(1200, easing = FastOutSlowInEasing)),
        label = "t"
    )
    val brush = Brush.linearGradient(
        colors = listOf(Color.White.copy(0.1f), Color.White.copy(0.4f), Color.White.copy(0.1f)),
        start = androidx.compose.ui.geometry.Offset(translate - 300f, translate - 300f),
        end = androidx.compose.ui.geometry.Offset(translate, translate)
    )
    val skeletonColor = Color(0xFFE2E8F0).copy(alpha = 0.6f)
    val isDark = LocalIsDarkTheme.current
    val bgColor = if (isDark) skeletonColor.copy(alpha = 0.15f) else skeletonColor
    val shape = RoundedCornerShape(4.dp)

    Box(
        modifier = modifier
            .fillMaxWidth()
            .height(120.dp)
            .pristineGlass(cornerRadius = 16.dp)
    ) {
        Row(
            modifier = Modifier.fillMaxSize().padding(16.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Box(Modifier.size(50.dp).background(bgColor, CircleShape).background(brush, CircleShape))
            Spacer(Modifier.width(16.dp))
            Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                Box(Modifier.width(140.dp).height(16.dp).background(bgColor, shape).background(brush, shape))
                Box(Modifier.width(80.dp).height(12.dp).background(bgColor.copy(alpha = 0.6f), shape).background(brush, shape))
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
// 3.  Modal Bottom Sheet (light glass)
// ═══════════════════════════════════════════════════════════════════

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun GlassModalBottomSheet(
    sheetState: SheetState,
    title: String,
    message: String,
    confirmLabel: String = "确认",
    dismissLabel: String = "取消",
    onDismiss: () -> Unit,
    onConfirm: () -> Unit
) {
    val isDark = LocalIsDarkTheme.current
    val textPrimary = if (isDark) TextPrimary else TextPrimary
    val textSecondary = if (isDark) TextSecondary else TextSecondary

    ModalBottomSheet(
        onDismissRequest = onDismiss,
        sheetState = sheetState,
        containerColor = Color.Transparent,
        dragHandle = null
    ) {
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 16.dp, vertical = 8.dp)
                .pristineGlass(cornerRadius = 28.dp)
                .padding(24.dp)
        ) {
            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                Box(Modifier.width(40.dp).height(4.dp).background(if (isDark) TextTertiary else Color(0xFFCBD5E1), CircleShape))
                Spacer(Modifier.height(24.dp))
                Text(title, fontSize = 20.sp, fontWeight = FontWeight.Bold, color = textPrimary)
                Spacer(Modifier.height(12.dp))
                Text(message, fontSize = 15.sp, color = textSecondary)
                Spacer(Modifier.height(28.dp))
                Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(16.dp)) {
                    OutlinedButton(onClick = onDismiss, modifier = Modifier.weight(1f)) {
                        Text(dismissLabel)
                    }
                    Button(onClick = onConfirm, modifier = Modifier.weight(1f)) {
                        Text(confirmLabel)
                    }
                }
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
// 4.  Empty state
// ═══════════════════════════════════════════════════════════════════

@Composable
fun GlassEmptyState(
    title: String,
    description: String = "",
    icon: @Composable () -> Unit = {
        Box(
            modifier = Modifier
                .size(80.dp)
                .background(if (LocalIsDarkTheme.current) AccentMint.copy(alpha = 0.15f) else Color(0xFFCCFBF1), CircleShape),
            contentAlignment = Alignment.Center
        ) {
            Icon(
                Icons.Default.CheckCircle, null,
                modifier = Modifier.size(40.dp),
                tint = if (LocalIsDarkTheme.current) AccentMint else DayAccent
            )
        }
    },
    modifier: Modifier = Modifier
) {
    val isDark = LocalIsDarkTheme.current
    val hColor = if (isDark) TextPrimary else TextPrimary
    val bColor = if (isDark) TextSecondary else TextSecondary

    Box(
        modifier = modifier
            .fillMaxWidth()
            .pristineGlass(cornerRadius = 32.dp)
            .padding(40.dp),
        contentAlignment = Alignment.Center
    ) {
        Column(horizontalAlignment = Alignment.CenterHorizontally) {
            icon()
            Spacer(Modifier.height(24.dp))
            Text(title, fontSize = 18.sp, fontWeight = FontWeight.Bold, color = hColor)
            if (description.isNotEmpty()) {
                Spacer(Modifier.height(8.dp))
                Text(
                    description, fontSize = 14.sp, color = bColor,
                    textAlign = TextAlign.Center
                )
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
// 5.  Permission guide
// ═══════════════════════════════════════════════════════════════════

@Composable
fun GlassPermissionGuide(
    step: Int,
    totalSteps: Int,
    title: String,
    description: String,
    onAuthorize: () -> Unit,
    onSkip: () -> Unit
) {
    val isDark = LocalIsDarkTheme.current
    val bg = if (isDark) DeepBackground else DayBg
    val hColor = if (isDark) TextPrimary else TextPrimary
    val bColor = if (isDark) TextSecondary else TextSecondary

    Box(
        modifier = Modifier.fillMaxSize().background(bg),
        contentAlignment = Alignment.Center
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(24.dp)
                .pristineGlass(cornerRadius = 32.dp)
                .padding(32.dp)
        ) {
            Text(
                "STEP $step / $totalSteps",
                color = DayAccent, fontWeight = FontWeight.Bold, fontSize = 12.sp
            )
            Spacer(Modifier.height(12.dp))
            Text(title, fontSize = 22.sp, fontWeight = FontWeight.Bold, color = hColor)
            Spacer(Modifier.height(12.dp))
            Text(description, color = bColor, fontSize = 15.sp)

            Spacer(Modifier.height(40.dp))

            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                repeat(totalSteps) { i ->
                    val active = i + 1 == step
                    Box(
                        modifier = Modifier
                            .width(if (active) 24.dp else 8.dp)
                            .height(8.dp)
                            .background(
                                if (active) DayAccent else if (isDark) TextTertiary else Color(0xFFCBD5E1),
                                CircleShape
                            )
                    )
                }
            }

            Spacer(Modifier.height(32.dp))

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    "暂不授权", modifier = Modifier.clickable(onClick = onSkip),
                    color = if (isDark) TextTertiary else Color(0xFF94A3B8), fontSize = 15.sp
                )
                Button(
                    onClick = onAuthorize,
                    colors = ButtonDefaults.buttonColors(containerColor = if (isDark) AccentPurple else DayAccent)
                ) {
                    Text("去授权", color = Color.White)
                }
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
// 6.  Collapsible Top App Bar (upgraded — takes collapsedFraction directly)
// ═══════════════════════════════════════════════════════════════════

@Composable
fun CollapsibleGlassTopBar(
    title: String,
    collapsedFraction: Float = 0f,
    navigationIcon: @Composable (() -> Unit)? = null,
    actions: @Composable (() -> Unit)? = null
) {
    val isDark = LocalIsDarkTheme.current
    val baseBgColor = if (isDark) Color(0xFF0F172A) else Color.White

    Box(
        modifier = Modifier
            .fillMaxWidth()
            .graphicsLayer { alpha = 1f }
            .background(baseBgColor.copy(alpha = 0.8f * collapsedFraction))
            .statusBarsPadding()
            .height(64.dp),
        contentAlignment = Alignment.CenterStart
    ) {
        Row(
            modifier = Modifier.fillMaxSize().padding(horizontal = 16.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            if (navigationIcon != null) {
                navigationIcon()
                Spacer(modifier = Modifier.width(12.dp))
            }
            Text(
                text = title,
                fontSize = (24 - (4 * collapsedFraction).toInt()).sp,
                fontWeight = FontWeight.Bold,
                color = if (isDark) Color.White else Color(0xFF0F172A)
            )
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
// 7.  Log viewer
// ═══════════════════════════════════════════════════════════════════


@Composable
fun GlassLogViewer(logs: List<LogEntry>) {
    val isDark = LocalIsDarkTheme.current
    val textColor = if (isDark) TextPrimary else TextPrimary

    Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .height(50.dp)
                .pristineGlass(cornerRadius = 14.dp)
                .padding(horizontal = 16.dp),
            contentAlignment = Alignment.CenterStart
        ) {
            Text("搜索或过滤诊断日志...", color = if (isDark) TextTertiary else Color(0xFF94A3B8), fontSize = 14.sp)
        }
        Spacer(Modifier.height(16.dp))

        LazyColumn(verticalArrangement = Arrangement.spacedBy(8.dp)) {
            items(logs) { log ->
                val tagColor = when (log.type) {
                    LogType.ERROR -> Color(0xFFEF4444)
                    LogType.WARN -> Color(0xFFF59E0B)
                    LogType.INFO -> if (isDark) TextTertiary else Color(0xFF64748B)
                }
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .pristineGlass(cornerRadius = 12.dp)
                        .padding(12.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Box(
                        modifier = Modifier
                            .background(tagColor.copy(alpha = 0.15f), RoundedCornerShape(6.dp))
                            .padding(horizontal = 8.dp, vertical = 4.dp)
                    ) {
                        Text(log.type.name, color = tagColor, fontWeight = FontWeight.Bold, fontSize = 11.sp)
                    }
                    Spacer(Modifier.width(12.dp))
                    Text(log.time, color = if (isDark) TextTertiary else Color(0xFF94A3B8), fontSize = 12.sp)
                    Spacer(Modifier.width(12.dp))
                    Text(log.message, color = textColor, fontSize = 13.sp)
                }
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
// 8.  Update banner
// ═══════════════════════════════════════════════════════════════════

@Composable
fun GlassUpdateBanner(
    version: String,
    description: String = "优化了系统底层检测逻辑与性能表现",
    onUpdate: () -> Unit,
    onClose: () -> Unit,
    modifier: Modifier = Modifier
) {
    var visible by remember { mutableStateOf(true) }
    if (!visible) return

    val isDark = LocalIsDarkTheme.current
    val hColor = if (isDark) TextPrimary else TextPrimary
    val bColor = if (isDark) TextSecondary else Color(0xFF64748B)

    Box(
        modifier = modifier
            .fillMaxWidth()
            .pristineGlass(cornerRadius = 20.dp)
            .padding(16.dp)
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Row(verticalAlignment = Alignment.CenterVertically, modifier = Modifier.weight(1f)) {
                Text("🚀", fontSize = 20.sp)
                Spacer(Modifier.width(12.dp))
                Column(modifier = Modifier.weight(1f)) {
                    Text("新版本 V$version 可用", fontWeight = FontWeight.Bold, fontSize = 15.sp, color = hColor)
                    Text(description, fontSize = 12.sp, color = bColor)
                }
            }
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text(
                    "更新", color = DayAccent, fontWeight = FontWeight.Bold, fontSize = 14.sp,
                    modifier = Modifier.clickable(onClick = onUpdate)
                )
                Spacer(Modifier.width(16.dp))
                Text(
                    "✕", color = if (isDark) TextTertiary else Color(0xFF94A3B8), fontSize = 14.sp,
                    modifier = Modifier.clickable { visible = false; onClose() }
                )
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
// 9.  Share report preview (full-screen overlay)
// ═══════════════════════════════════════════════════════════════════

@Composable
fun GlassShareReportPreview(
    score: Int,
    statusLabel: String,
    statusSummary: String,
    onClose: () -> Unit = {},
    onShare: () -> Unit = {}
) {
    val isDark = LocalIsDarkTheme.current
    val bgColor = if (isDark) Color(0xFF0B0B10) else LightBackground

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(bgColor)
            .padding(24.dp)
    ) {
        Column(
            modifier = Modifier.fillMaxSize(),
            verticalArrangement = Arrangement.SpaceBetween
        ) {
            Column {
                Text(
                    "APEX ENVIRONMENT SECURITY",
                    color = if (isDark) Color.White.copy(alpha = 0.4f) else Color.Black.copy(alpha = 0.4f), fontSize = 11.sp,
                    fontWeight = FontWeight.Bold
                )
                Spacer(Modifier.height(8.dp))
                Text("环境扫描摘要报告", color = if (isDark) Color.White else Color.Black, fontSize = 24.sp, fontWeight = FontWeight.Bold)
                Spacer(Modifier.height(40.dp))
                Text(
                    score.toString(), fontSize = 72.sp, fontWeight = FontWeight.Black,
                    color = AccentPurple
                )
                Text(
                    if (score > 90) "STATUS: SECURE" else "STATUS: WARNING",
                    color = if (isDark) Color.White else Color.Black, fontSize = 18.sp
                )
            }
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(100.dp)
                    .pristineGlass(cornerRadius = 16.dp)
                    .padding(16.dp),
                contentAlignment = Alignment.CenterStart
            ) {
                Text(
                    statusSummary,
                    color = if (isDark) Color.White.copy(alpha = 0.7f) else Color.Black.copy(alpha = 0.7f), fontSize = 13.sp
                )
            }
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(16.dp)
            ) {
                OutlinedButton(onClick = onClose, modifier = Modifier.weight(1f)) {
                    Text("关闭", color = if (isDark) Color.White else Color.Black)
                }
                Button(
                    onClick = onShare,
                    modifier = Modifier.weight(1f),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentPurple)
                ) {
                    Text("分享报告", color = Color.White)
                }
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
// 10. Custom Snackbar
// ═══════════════════════════════════════════════════════════════════

enum class SnackbarType { SUCCESS, WARNING, ERROR }

@Composable
fun GlassCustomSnackbar(message: String, type: SnackbarType) {
    val indicatorColor = when (type) {
        SnackbarType.SUCCESS -> Color(0xFF10B981)
        SnackbarType.WARNING -> Color(0xFFF59E0B)
        SnackbarType.ERROR -> Color(0xFFEF4444)
    }
    val isDark = LocalIsDarkTheme.current
    val textColor = if (isDark) TextPrimary else Color(0xFF1E293B)

    Box(
        modifier = Modifier
            .fillMaxWidth()
            .padding(16.dp)
            .pristineGlass(cornerRadius = 16.dp)
    ) {
        Row(modifier = Modifier.fillMaxSize(), verticalAlignment = Alignment.CenterVertically) {
            Box(Modifier.width(6.dp).height(50.dp).background(indicatorColor, RoundedCornerShape(topStart = 16.dp, bottomStart = 16.dp)))
            Spacer(Modifier.width(16.dp))
            Text(message, color = textColor, fontSize = 14.sp, fontWeight = FontWeight.Medium)
        }
    }
}
