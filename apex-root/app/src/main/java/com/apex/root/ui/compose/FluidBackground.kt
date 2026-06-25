package com.apex.root.ui.compose

import androidx.compose.animation.core.*
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.blur
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp

private val fluidEase = CubicBezierEasing(0.42f, 0f, 0.58f, 1f)

data class OrbConfig(
    val size: Dp,
    val blurRadius: Dp,
    val color: Color,
    val alpha: Float,
    val scale: Float,
    val xRange: ClosedFloatingPointRange<Float>,
    val yRange: ClosedFloatingPointRange<Float>,
    val durationX: Int,
    val durationY: Int,
    val alignment: Alignment
)

private val orbs = listOf(
    OrbConfig(340.dp, 120.dp, AccentPurple, 0.55f, 1.06f, -70f..70f, -50f..50f, 9000, 11000, Alignment.TopStart),
    OrbConfig(300.dp, 100.dp, AccentBlue, 0.50f, 1.00f, 60f..(-60f), 50f..(-40f), 11000, 13000, Alignment.CenterEnd),
    OrbConfig(320.dp, 130.dp, Color(0xFF7C4DFF), 0.45f, 1.04f, 0f..90f, 0f..(-70f), 14000, 15000, Alignment.BottomCenter),
    OrbConfig(260.dp, 110.dp, AccentGold, 0.35f, 1.00f, -40f..40f, -60f..30f, 12000, 14000, Alignment.CenterStart)
)

@Composable
fun FluidBackground(modifier: Modifier = Modifier) {
    val transition = rememberInfiniteTransition()

    Box(modifier = modifier.fillMaxSize()) {
        orbs.forEachIndexed { i, cfg ->
            val offsetX by transition.animateFloat(
                initialValue = cfg.xRange.start,
                targetValue = cfg.xRange.endInclusive,
                animationSpec = infiniteRepeatable(
                    tween(cfg.durationX, easing = fluidEase),
                    repeatMode = RepeatMode.Reverse
                )
            )
            val offsetY by transition.animateFloat(
                initialValue = cfg.yRange.start,
                targetValue = cfg.yRange.endInclusive,
                animationSpec = infiniteRepeatable(
                    tween(cfg.durationY, easing = fluidEase),
                    repeatMode = RepeatMode.Reverse
                )
            )
            Orb(
                size = cfg.size, blurRadius = cfg.blurRadius,
                color1 = cfg.color, alpha = cfg.alpha, scale = cfg.scale,
                offsetX = offsetX, offsetY = offsetY,
                alignment = cfg.alignment
            )
        }
    }
}

@Composable
private fun Orb(
    size: Dp, blurRadius: Dp,
    color1: Color, alpha: Float, scale: Float,
    offsetX: Float, offsetY: Float,
    alignment: Alignment
) {
    Box(modifier = Modifier.fillMaxSize(), contentAlignment = alignment) {
        Box(
            modifier = Modifier
                .size(size * scale)
                .offset(x = offsetX.dp, y = offsetY.dp)
                .blur(blurRadius)
                .background(
                    Brush.radialGradient(listOf(color1.copy(alpha = alpha), Color.Transparent)),
                    CircleShape
                )
        )
    }
}
