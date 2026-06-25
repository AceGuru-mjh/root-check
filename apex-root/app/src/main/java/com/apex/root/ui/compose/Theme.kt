package com.apex.root.ui.compose

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.staticCompositionLocalOf
import androidx.compose.ui.graphics.Color

val DeepBackground = Color(0xFF0B0B10)
val DeepSurface = Color(0xFF14141C)
val DeepSurfaceVariant = Color(0xFF1C1C28)
val DeepSurfaceBright = Color(0xFF24243A)
val AccentPurple = Color(0xFF7C5CFC)
val AccentPurpleSoft = Color(0xFF9B7EFF)
val AccentBlue = Color(0xFF5CA0FC)
val AccentBlueSoft = Color(0xFF7EB8FF)
val AccentGold = Color(0xFFFCD45C)
val AccentGoldSoft = Color(0xFFFFE082)
val AccentMint = Color(0xFF4CAF50)
val AccentMintSoft = Color(0xFF76D27A)
val ErrorRed = Color(0xFFFF5252)
val ErrorRedSoft = Color(0xFFFF7A7A)
var TextPrimary: Color = Color(0xFFECECF5)
var TextSecondary: Color = Color(0xFF9A9AB0)
var TextTertiary: Color = Color(0xFF5C5C78)
val TermuxBg = Color(0xFF08080E)
val TermuxGreen = Color(0xFF66FF88)

val LightBackground = Color(0xFFF8F7FF)
val LightSurface = Color(0xFFFFFFFF)
val LightSurfaceVariant = Color(0xFFF0EEF8)
val LightTextPrimary = Color(0xFF1A1A2E)
val LightTextSecondary = Color(0xFF6B6B80)
val LightTextTertiary = Color(0xFF9D9DB0)

val PastelPurple = Color(0xFFD4C5FF)
val PastelBlue = Color(0xFFC5E0FF)
val PastelGold = Color(0xFFFFE8A0)
val PastelMint = Color(0xFFC5F0D5)
val PastelRed = Color(0xFFFFC5C5)

val GlassBaseDark = Color.White.copy(alpha = 0.07f)
val GlassBorderDark = Color.White.copy(alpha = 0.22f)
val GlassBaseLight = Color.Black.copy(alpha = 0.03f)
val GlassBorderLight = Color.Black.copy(alpha = 0.08f)

val LocalIsDarkTheme = staticCompositionLocalOf { true }

private val DarkColorScheme = darkColorScheme(
    primary = AccentPurple,
    secondary = AccentBlue,
    tertiary = AccentGold,
    background = DeepBackground,
    surface = DeepSurface,
    surfaceVariant = DeepSurfaceVariant,
    error = ErrorRed,
    onPrimary = Color.White,
    onSecondary = Color.White,
    onTertiary = Color(0xFF1A1A1A),
    onBackground = TextPrimary,
    onSurface = TextPrimary,
    onSurfaceVariant = TextSecondary,
    onError = Color.White,
    outline = DeepSurfaceBright
)

private val LightColorScheme = lightColorScheme(
    primary = AccentPurple,
    secondary = AccentBlue,
    tertiary = AccentGold,
    background = LightBackground,
    surface = LightSurface,
    surfaceVariant = LightSurfaceVariant,
    error = ErrorRed,
    onPrimary = Color.White,
    onSecondary = Color.White,
    onTertiary = Color(0xFF1A1A1A),
    onBackground = LightTextPrimary,
    onSurface = LightTextPrimary,
    onSurfaceVariant = LightTextSecondary,
    onError = Color.White,
    outline = Color(0xFFE0E0F0)
)

@Composable
fun ApexRootTheme(
    darkTheme: Boolean = isSystemInDarkTheme(),
    content: @Composable () -> Unit
) {
    TextPrimary = if (darkTheme) Color(0xFFECECF5) else Color(0xFF1A1A2E)
    TextSecondary = if (darkTheme) Color(0xFF9A9AB0) else Color(0xFF6B6B80)
    TextTertiary = if (darkTheme) Color(0xFF5C5C78) else Color(0xFF9D9DB0)

    CompositionLocalProvider(LocalIsDarkTheme provides darkTheme) {
        MaterialTheme(
            colorScheme = if (darkTheme) DarkColorScheme else LightColorScheme,
            typography = Typography(),
            content = content
        )
    }
}
