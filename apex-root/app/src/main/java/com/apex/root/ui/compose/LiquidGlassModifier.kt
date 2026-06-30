package com.apex.root.ui.compose

import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.drawWithContent
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.runtime.Composable
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp

@Composable
fun Modifier.liquidGlass(
    cornerRadius: Dp = 24.dp,
    baseColor: Color = if (LocalIsDarkTheme.current) Color.White.copy(alpha = 0.07f) else Color.Black.copy(alpha = 0.04f),
    borderColor: Color = if (LocalIsDarkTheme.current) Color.White.copy(alpha = 0.22f) else Color.Black.copy(alpha = 0.10f),
    strokeWidth: Dp = 1.dp
): Modifier {
    val density = LocalDensity.current
    val cornerPx = with(density) { cornerRadius.toPx() }
    val strokePx = with(density) { strokeWidth.toPx() }
    val isDark = LocalIsDarkTheme.current

    return this.drawWithContent {
        drawContent()

        val cx = size.width * 0.28f
        val cy = size.height * 0.18f
        val r = size.maxDimension * 0.9f
        val fillBrush = Brush.radialGradient(
            colors = listOf(baseColor.copy(alpha = 0.18f), baseColor.copy(alpha = 0.03f)),
            center = Offset(cx, cy), radius = r
        )

        val hl = if (isDark) Color.White else Color.Black
        val strokeBrush = Brush.linearGradient(
            colors = listOf(
                hl.copy(alpha = 0.50f),
                hl.copy(alpha = 0.06f),
                borderColor,
                AccentPurple.copy(alpha = 0.10f),
                hl.copy(alpha = 0.25f)
            ),
            start = Offset.Zero, end = Offset(size.width, size.height)
        )

        val cr = CornerRadius(cornerPx, cornerPx)
        drawRoundRect(brush = fillBrush, cornerRadius = cr)
        drawRoundRect(brush = strokeBrush, cornerRadius = cr, style = Stroke(width = strokePx))
    }
}

@Composable
fun Modifier.liquidGlassIcon(
    cornerRadius: Dp = 10.dp,
    baseColor: Color = Color.White.copy(alpha = 0.08f)
): Modifier {
    val hl = if (LocalIsDarkTheme.current) Color.White else Color.Black.copy(alpha = 0.4f)
    return this.drawWithContent {
        drawContent()

        val cx = size.width * 0.35f
        val cy = size.height * 0.3f
        val r = size.maxDimension * 0.85f

        val fillBrush = Brush.radialGradient(
            colors = listOf(baseColor.copy(alpha = 0.16f), baseColor.copy(alpha = 0.03f)),
            center = Offset(cx, cy), radius = r
        )
        val strokeBrush = Brush.linearGradient(
            colors = listOf(hl.copy(alpha = 0.35f), hl.copy(alpha = 0.04f)),
            start = Offset.Zero, end = Offset(size.width, size.height)
        )

        val cr = CornerRadius(cornerRadius.toPx(), cornerRadius.toPx())
        drawRoundRect(brush = fillBrush, cornerRadius = cr)
        drawRoundRect(brush = strokeBrush, cornerRadius = cr, style = Stroke(width = 0.8.dp.toPx()))
    }
}
