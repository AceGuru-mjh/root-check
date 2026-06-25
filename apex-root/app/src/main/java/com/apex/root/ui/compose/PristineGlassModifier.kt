package com.apex.root.ui.compose

import android.os.Build
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.drawWithContent
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Shader
import androidx.compose.ui.graphics.asComposeRenderEffect
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import kotlin.math.sqrt

@Composable
fun Modifier.pristineGlass(
    cornerRadius: Dp = 24.dp
): Modifier {
    val isDark = isSystemInDarkTheme()
    val baseColor = if (isDark) Color(0xFF1E293B).copy(alpha = 0.25f) else Color.White.copy(alpha = 0.12f)
    val strokeA = if (isDark) Color.White.copy(alpha = 0.2f) else Color.White.copy(alpha = 0.6f)
    val strokeB = if (isDark) Color.White.copy(alpha = 0.05f) else Color.White.copy(alpha = 0.15f)

    return this
        .graphicsLayer {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                renderEffect = android.graphics.BlurEffect.createBlurEffect(35f, 35f, Shader.TileMode.CLAMP).asComposeRenderEffect()
            }
        }
        .drawWithContent {
            val cr = androidx.compose.ui.geometry.CornerRadius(cornerRadius.toPx())
            drawRoundRect(color = baseColor, cornerRadius = cr)
            drawContent()
            drawRoundRect(
                brush = Brush.linearGradient(listOf(strokeA, strokeB)),
                cornerRadius = cr,
                style = Stroke(width = 1.2.dp.toPx())
            )
        }
}
