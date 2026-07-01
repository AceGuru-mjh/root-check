package com.apex.root.ui.compose

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.staticCompositionLocalOf
import androidx.compose.ui.graphics.Color

// ─── 静态颜色（不随主题变化）──────────────────────────
val DeepBackground = Color(0xFF0B0B10)
val DeepSurface = Color(0xFF14141C)
val DeepSurfaceVariant = Color(0xFF1C1C28)
val DeepSurfaceBright = Color(0xFF24243A)
val AccentPurple = Color(0xFF7C5CFC)
val AccentPurpleSoft = Color(0xFF9B7EFF)
val AccentBlue = Color(0xFF5CA0FC)
val AccentBlueSoft = Color(0xFF7EB8FF)
val AccentCyan = Color(0xFF00BCD4)
val AccentGold = Color(0xFFFCD45C)
val AccentGoldSoft = Color(0xFFFFE082)
val AccentMint = Color(0xFF4CAF50)
val AccentMintSoft = Color(0xFF76D27A)
val ErrorRed = Color(0xFFFF5252)
val ErrorRedSoft = Color(0xFFFF7A7A)
val TermuxBg = Color(0xFF08080E)
val TermuxGreen = Color(0xFF66FF88)

val LightBackground = Color(0xFFF8F7FF)
val LightSurface = Color(0xFFFFFFFF)
val LightSurfaceVariant = Color(0xFFF0EEF8)

val PastelPurple = Color(0xFFD4C5FF)
val PastelBlue = Color(0xFFC5E0FF)
val PastelGold = Color(0xFFFFE8A0)
val PastelMint = Color(0xFFC5F0D5)
val PastelRed = Color(0xFFFFC5C5)

val GlassBaseDark = Color.White.copy(alpha = 0.07f)
val GlassBorderDark = Color.White.copy(alpha = 0.22f)
val GlassBaseLight = Color.Black.copy(alpha = 0.03f)
val GlassBorderLight = Color.Black.copy(alpha = 0.08f)

// ─── 主题感知颜色 — 通过 CompositionLocal 传递 ──────
// 修复：原来的 var TextPrimary 等是 mutable 全局变量，
// Compose 不跟踪变化，主题切换后颜色不会刷新。
// 改为 CompositionLocal，确保主题切换时正确 recompose。

data class ApexTextColors(
    val primary: Color,
    val secondary: Color,
    val tertiary: Color
)

// 修复：提升颜色对比度（WCAG AA 标准）
// Dark: secondary 从 9A9AB0 提亮到 B0B0C8, tertiary 从 5C5C78 提亮到 8088A0
private val DarkTextColors = ApexTextColors(
    primary = Color(0xFFECECF5),
    secondary = Color(0xFFB0B0C8),
    tertiary = Color(0xFF8088A0)
)

// 修复：Light 模式 secondary 从 6B6B80 加深到 4A4A5C, tertiary 从 9D9DB0 加深到 6B6B80
private val LightTextColors = ApexTextColors(
    primary = Color(0xFF1A1A2E),
    secondary = Color(0xFF4A4A5C),
    tertiary = Color(0xFF6B6B80)
)

val LocalApexTextColors = staticCompositionLocalOf { DarkTextColors }

/**
 * 便捷属性 — 在 @Composable 中使用 TextPrimary / TextSecondary / TextTertiary
 * 这些现在通过 CompositionLocal 读取，会随主题自动刷新。
 */
val TextPrimary: Color
    @Composable get() = LocalApexTextColors.current.primary

val TextSecondary: Color
    @Composable get() = LocalApexTextColors.current.secondary

val TextTertiary: Color
    @Composable get() = LocalApexTextColors.current.tertiary

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
    onBackground = Color(0xFFECECF5),
    onSurface = Color(0xFFECECF5),
    onSurfaceVariant = Color(0xFF9A9AB0),
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
    onBackground = Color(0xFF1A1A2E),
    onSurface = Color(0xFF1A1A2E),
    onSurfaceVariant = Color(0xFF6B6B80),
    onError = Color.White,
    outline = Color(0xFFE0E0F0)
)

@Composable
fun ApexRootTheme(
    darkTheme: Boolean = isSystemInDarkTheme(),
    content: @Composable () -> Unit
) {
    val textColors = if (darkTheme) DarkTextColors else LightTextColors

    CompositionLocalProvider(
        LocalIsDarkTheme provides darkTheme,
        LocalApexTextColors provides textColors
    ) {
        MaterialTheme(
            colorScheme = if (darkTheme) DarkColorScheme else LightColorScheme,
            typography = Typography(),
            content = content
        )
    }
}
