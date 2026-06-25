package com.apex.root.ui.compose

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.blur
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
        Box(
            modifier = Modifier
                .fillMaxSize()
                .background(Brush.linearGradient(colors))
                .blur(70.dp)
        )
        Box(
            modifier = Modifier
                .fillMaxSize()
                .background(overlay)
        ) {
            content()
        }
    }
}
