package com.apex.root.ui.compose

import android.os.Build
import androidx.compose.foundation.background
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
 * 修复：将 Compose 软件 blur 从 70dp 降至 20dp（低端机可承受），
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
                        Modifier.blur(20.dp) // 从 70dp 降至 20dp
                    }
                )
                .background(Brush.linearGradient(colors))
        )
        // 半透明遮罩 + 内容
        Box(
            modifier = Modifier
                .fillMaxSize()
                .background(overlay)
        ) {
            content()
        }
    }
}
