package com.apex.root.ui.compose

import android.os.Build
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.pointerInput
import androidx.compose.foundation.layout.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.blur
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import android.graphics.Shader
import androidx.compose.ui.graphics.asComposeRenderEffect
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.unit.dp

object PageFluidColors {
    val dashboard = listOf(AccentPurple.copy(alpha = 0.6f), AccentBlue.copy(alpha = 0.4f), DeepBackground)
    val report = listOf(AccentMint.copy(alpha = 0.5f), AccentBlue.copy(alpha = 0.3f), DeepBackground)
    val alert = listOf(ErrorRed.copy(alpha = 0.5f), AccentGold.copy(alpha = 0.3f), DeepBackground)
    val settings = listOf(AccentPurple.copy(alpha = 0.5f), AccentGold.copy(alpha = 0.3f), DeepBackground)

    val dashboardLight = listOf(PastelPurple, PastelBlue, LightBackground)
    val reportLight = listOf(PastelMint, PastelBlue, LightBackground)
    val alertLight = listOf(PastelRed, PastelGold, LightBackground)
    val settingsLight = listOf(PastelPurple, PastelGold, LightBackground)
}

/**
 * 毛玻璃容器。
 *
 * 修复 v1.0.7:
 *   - 背景 Box 加 pointerInput(Unit) {} 消费但不拦截触摸事件,避免
 *     graphicsLayer renderEffect 导致父 Box 拦截子 Column 的滚动手势
 *   - 内容 Box 改为 fillMaxSize 但不拦截触摸,让子 verticalScroll 正常工作
 *   - 移除背景 Box 的 clickable/任何手势修饰符
 *
 * 修复历史:将 Compose 软件 blur 从 70dp 降至 20dp（低端机可承受），
 * Android 12+ 优先使用硬件 RenderEffect（性能更好），
 * Android 12 以下回退到 Compose 软件 blur（半径更低）。
 */
@Composable
fun LiquidGlassContainer(
    fluidColorsDark: List<Color> = PageFluidColors.dashboard,
    fluidColorsLight: List<Color> = PageFluidColors.dashboardLight,
    content: @Composable BoxScope.() -> Unit
) {
    val isDark = LocalIsDarkTheme.current
    val colors = if (isDark) fluidColorsDark else fluidColorsLight
    val overlay = if (isDark) DeepBackground.copy(alpha = 0.88f) else Color.White.copy(alpha = 0.82f)

    Box(modifier = Modifier.fillMaxSize()) {
        // 背景渐变层 — Android 12+ 用硬件模糊，低于 12 用轻量软件模糊
        // 修复 v1.0.7: 背景层不应消费触摸事件,否则会拦截子内容的滚动手势
        Box(
            modifier = Modifier
                .fillMaxSize()
                .then(
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                        Modifier.graphicsLayer {
                            renderEffect = android.graphics.RenderEffect
                                .createBlurEffect(20f, 20f, Shader.TileMode.CLAMP)
                                .asComposeRenderEffect()
                        }
                    } else {
                        Modifier.blur(20.dp)
                    }
                )
                .background(Brush.linearGradient(colors))
                // 关键修复: 背景层明确不消费任何输入事件,让触摸/滚动穿透到内容层
                .pointerInput(Unit) { /* 不消费任何手势,仅占位 */ }
        )
        // 半透明遮罩 + 内容
        // 修复 v1.0.7: 内容 Box 也不应拦截手势,让子 Composable (如 Column.verticalScroll)
        // 完全接管滚动手势
        Box(
            modifier = Modifier
                .fillMaxSize()
                .background(overlay)
        ) {
            content()
        }
    }
}
