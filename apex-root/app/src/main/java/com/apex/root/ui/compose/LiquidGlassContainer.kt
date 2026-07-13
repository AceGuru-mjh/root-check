package com.apex.root.ui.compose

import android.os.Build
import androidx.compose.foundation.layout.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color

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
 * 毛玻璃容器 — v1.0.1 彻底重写。
 *
 * v1.0.1 根因修复:
 * 旧实现用两个独立的 Box (背景 Box + 内容 Box) 叠放。背景 Box 上的
 * graphicsLayer { renderEffect = BlurEffect } 在 Android 12+ 会创建一个
 * 硬件加速渲染层,该层会被系统视为一个独立的可交互区域,在某些设备
 * (尤其是 MIUI / ColorOS / OneUI 等定制 ROM) 上会拦截触摸事件,
 * 导致前景内容 Box 中的按钮/开关/滚动都收不到点击。
 *
 * 修复方案:
 * 用单个 Box + drawBehind 在同一图层绘制背景渐变 + 半透明遮罩 + 内容。
 * 不创建任何额外的渲染层,触摸事件直接到达内容中的按钮。
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

    // v1.0.1: 单个 Box,背景用 drawBehind 绘制 (不创建额外渲染层,不拦截触摸)
    Box(
        modifier = Modifier
            .fillMaxSize()
            .drawBehind {
                // 绘制背景渐变
                drawRect(Brush.linearGradient(colors))
                // 绘制半透明遮罩
                drawRect(overlay)
            }
    ) {
        content()
    }
}
