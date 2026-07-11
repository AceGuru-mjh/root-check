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
 * 修复 v1.0.9:
 *   - 完全移除背景 Box 的 pointerInput(Unit){}。
 *     v1.0.7 添加的 pointerInput(Unit){} 即使块为空,也会注册一个指针输入处理
 *     器,在 Compose 的事件分发中会拦截所有指针事件,导致子内容的按钮点击
 *     和滚动手势都收不到事件 → "首次进入点按钮没反应"。
 *   - 在 Compose 中,Box 默认不拦截触摸事件 (除非加了 clickable/pointerInput
 *     等手势修饰符)。graphicsLayer/renderEffect/background 都不会拦截事件。
 *     因此背景层无需任何额外修饰符来"放行"触摸事件。
 *   - 真正的滚动问题 (v1.0.7 报告) 根因是 Scaffold 的 padding + Box.fillMaxSize
 *     导致 Column 高度计算异常,已通过 DashboardScreen 的布局修复解决。
 *
 * 修复历史:
 *   - v1.0.7: 加 pointerInput(Unit){} (错误修复,反而导致按钮失效)
 *   - 将 Compose 软件 blur 从 70dp 降至 20dp（低端机可承受），
 *     Android 12+ 优先使用硬件 RenderEffect（性能更好），
 *     Android 12 以下回退到 Compose 软件 blur（半径更低）。
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
        // 注意: 不加任何 pointerInput/clickable,Box 默认不拦截触摸事件,
        // 事件会自然传递到上层的 content Box 中的按钮/滚动容器。
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
        )
        // 半透明遮罩 + 内容
        // 内容 Box 也不加任何手势修饰符,让子 Composable 完全接管触摸事件
        Box(
            modifier = Modifier
                .fillMaxSize()
                .background(overlay)
        ) {
            content()
        }
    }
}
