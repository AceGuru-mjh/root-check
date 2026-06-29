package com.apex.root.ui.compose

import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.drawWithContent
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp

/**
 * 毛玻璃修饰符。
 *
 * 修复：移除了 35px 硬件 BlurEffect（每个元素独立创建模糊通道，
 * 大量使用时 GPU 过载，Android 12 以下完全无模糊导致样式不一致）。
 *
 * 改用半透明背景 + 边框模拟毛玻璃效果，视觉上更统一，性能更好。
 */
@Composable
fun Modifier.pristineGlass(
    cornerRadius: Dp = 24.dp,
    baseColor: Color? = null,
): Modifier {
    val isDark = LocalIsDarkTheme.current
    val resolvedBaseColor = baseColor
        ?: if (isDark) Color(0xFF1E293B).copy(alpha = 0.25f) else Color.White.copy(alpha = 0.12f)
    val strokeA = if (isDark) Color.White.copy(alpha = 0.2f) else Color.White.copy(alpha = 0.6f)
    val strokeB = if (isDark) Color.White.copy(alpha = 0.05f) else Color.White.copy(alpha = 0.15f)

    return this
        .drawWithContent {
            val cr = androidx.compose.ui.geometry.CornerRadius(cornerRadius.toPx())
            drawRoundRect(color = resolvedBaseColor, cornerRadius = cr)
            drawContent()
            drawRoundRect(
                brush = Brush.linearGradient(listOf(strokeA, strokeB)),
                cornerRadius = cr,
                style = Stroke(width = 1.2.dp.toPx())
            )
        }
}
