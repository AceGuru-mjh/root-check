package com.apex.root.ui.compose

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.staticCompositionLocalOf
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.sp

// ── Dark Theme Colors ──
val DeepBackground = Color(0xFF0B0B10)
val DeepSurface = Color(0xFF14141C)
val DeepSurfaceVariant = Color(0xFF1C1C28)
val DeepSurfaceBright = Color(0xFF24243A)

// Primary Brand Colors
val AccentPurple = Color(0xFF7C5CFC)
val AccentPurpleSoft = Color(0xFF9B7EFF)
val AccentPurpleContainer = Color(0xFF2D2440)
val AccentPurpleOnContainer = Color(0xFFE8DFFF)

val AccentBlue = Color(0xFF5CA0FC)
val AccentBlueSoft = Color(0xFF7EB8FF)
val AccentBlueContainer = Color(0xFF1F2F40)
val AccentBlueOnContainer = Color(0xFFD6E8FF)

val AccentGold = Color(0xFFFCD45C)
val AccentGoldSoft = Color(0xFFFFE082)
val AccentGoldContainer = Color(0xFF3D341F)
val AccentGoldOnContainer = Color(0xFFFFEAB8)

val AccentMint = Color(0xFF4CAF50)
val AccentMintSoft = Color(0xFF76D27A)
val AccentMintContainer = Color(0xFF1F3420)
val AccentMintOnContainer = Color(0xFFBCEBC0)

val ErrorRed = Color(0xFFFF5252)
val ErrorRedSoft = Color(0xFFFF7A7A)
val ErrorRedContainer = Color(0xFF401F1F)
val ErrorRedOnContainer = Color(0xFFFFDAD6)

// Text Colors (Dark)
var TextPrimary: Color = Color(0xFFECECF5)
var TextSecondary: Color = Color(0xFF9A9AB0)
var TextTertiary: Color = Color(0xFF5C5C78)
var TextInverse: Color = Color(0xFF1A1A2E)

// Special
val TermuxBg = Color(0xFF08080E)
val TermuxGreen = Color(0xFF66FF88)

// ── Light Theme Colors ──
val LightBackground = Color(0xFFF8F7FF)
val LightSurface = Color(0xFFFFFFFF)
val LightSurfaceVariant = Color(0xFFF0EEF8)
val LightSurfaceContainer = Color(0xFFF5F4FA)

val LightTextPrimary = Color(0xFF1A1A2E)
val LightTextSecondary = Color(0xFF6B6B80)
val LightTextTertiary = Color(0xFF9D9DB0)

// Pastel Colors (Light theme accents)
val PastelPurple = Color(0xFFD4C5FF)
val PastelPurpleContainer = Color(0xFFEDE8FF)
val PastelBlue = Color(0xFFC5E0FF)
val PastelBlueContainer = Color(0xFFE8F2FF)
val PastelGold = Color(0xFFFFE8A0)
val PastelGoldContainer = Color(0xFFFFF4D4)
val PastelMint = Color(0xFFC5F0D5)
val PastelMintContainer = Color(0xFFE8F8ED)
val PastelRed = Color(0xFFFFC5C5)
val PastelRedContainer = Color(0xFFFFE8E4)

// Glass Effects
val GlassBaseDark = Color.White.copy(alpha = 0.07f)
val GlassBorderDark = Color.White.copy(alpha = 0.22f)
val GlassBaseLight = Color.Black.copy(alpha = 0.03f)
val GlassBorderLight = Color.Black.copy(alpha = 0.08f)

val LocalIsDarkTheme = staticCompositionLocalOf { true }

// M3 Color Schemes
private val DarkColorScheme = darkColorScheme(
    primary = AccentPurple,
    onPrimary = Color.White,
    primaryContainer = AccentPurpleContainer,
    onPrimaryContainer = AccentPurpleOnContainer,
    
    secondary = AccentBlue,
    onSecondary = Color.White,
    secondaryContainer = AccentBlueContainer,
    onSecondaryContainer = AccentBlueOnContainer,
    
    tertiary = AccentGold,
    onTertiary = Color(0xFF1A1A1A),
    tertiaryContainer = AccentGoldContainer,
    onTertiaryContainer = AccentGoldOnContainer,
    
    error = ErrorRed,
    onError = Color.White,
    errorContainer = ErrorRedContainer,
    onErrorContainer = ErrorRedOnContainer,
    
    background = DeepBackground,
    onBackground = TextPrimary,
    
    surface = DeepSurface,
    onSurface = TextPrimary,
    surfaceVariant = DeepSurfaceVariant,
    onSurfaceVariant = TextSecondary,
    surfaceContainer = DeepSurfaceBright,
    
    outline = Color(0xFF4A4A5E),
    outlineVariant = Color(0xFF3A3A4E)
)

private val LightColorScheme = lightColorScheme(
    primary = AccentPurple,
    onPrimary = Color.White,
    primaryContainer = PastelPurpleContainer,
    onPrimaryContainer = AccentPurple,
    
    secondary = AccentBlue,
    onSecondary = Color.White,
    secondaryContainer = PastelBlueContainer,
    onSecondaryContainer = AccentBlue,
    
    tertiary = AccentGold,
    onTertiary = Color(0xFF1A1A1A),
    tertiaryContainer = PastelGoldContainer,
    onTertiaryContainer = Color(0xFF3D341F),
    
    error = ErrorRed,
    onError = Color.White,
    errorContainer = PastelRedContainer,
    onErrorContainer = Color(0xFF401F1F),
    
    background = LightBackground,
    onBackground = LightTextPrimary,
    
    surface = LightSurface,
    onSurface = LightTextPrimary,
    surfaceVariant = LightSurfaceVariant,
    onSurfaceVariant = LightTextSecondary,
    surfaceContainer = LightSurfaceContainer,
    
    outline = Color(0xFFC5C5D8),
    outlineVariant = Color(0xFFE0E0F0)
)

// M3 Typography
private val AppTypography = Typography(
    displayLarge = androidx.compose.ui.text.TextStyle(
        fontFamily = FontFamily.SansSerif,
        fontWeight = FontWeight.Normal,
        fontSize = 57.sp,
        lineHeight = 64.sp,
        letterSpacing = (-0.25).sp
    ),
    displayMedium = androidx.compose.ui.text.TextStyle(
        fontFamily = FontFamily.SansSerif,
        fontWeight = FontWeight.Normal,
        fontSize = 45.sp,
        lineHeight = 52.sp,
        letterSpacing = 0.sp
    ),
    displaySmall = androidx.compose.ui.text.TextStyle(
        fontFamily = FontFamily.SansSerif,
        fontWeight = FontWeight.Normal,
        fontSize = 36.sp,
        lineHeight = 44.sp,
        letterSpacing = 0.sp
    ),
    headlineLarge = androidx.compose.ui.text.TextStyle(
        fontFamily = FontFamily.SansSerif,
        fontWeight = FontWeight.SemiBold,
        fontSize = 32.sp,
        lineHeight = 40.sp,
        letterSpacing = 0.sp
    ),
    headlineMedium = androidx.compose.ui.text.TextStyle(
        fontFamily = FontFamily.SansSerif,
        fontWeight = FontWeight.SemiBold,
        fontSize = 28.sp,
        lineHeight = 36.sp,
        letterSpacing = 0.sp
    ),
    headlineSmall = androidx.compose.ui.text.TextStyle(
        fontFamily = FontFamily.SansSerif,
        fontWeight = FontWeight.SemiBold,
        fontSize = 24.sp,
        lineHeight = 32.sp,
        letterSpacing = 0.sp
    ),
    titleLarge = androidx.compose.ui.text.TextStyle(
        fontFamily = FontFamily.SansSerif,
        fontWeight = FontWeight.Bold,
        fontSize = 22.sp,
        lineHeight = 28.sp,
        letterSpacing = 0.sp
    ),
    titleMedium = androidx.compose.ui.text.TextStyle(
        fontFamily = FontFamily.SansSerif,
        fontWeight = FontWeight.SemiBold,
        fontSize = 16.sp,
        lineHeight = 24.sp,
        letterSpacing = 0.15.sp
    ),
    titleSmall = androidx.compose.ui.text.TextStyle(
        fontFamily = FontFamily.SansSerif,
        fontWeight = FontWeight.Medium,
        fontSize = 14.sp,
        lineHeight = 20.sp,
        letterSpacing = 0.1.sp
    ),
    bodyLarge = androidx.compose.ui.text.TextStyle(
        fontFamily = FontFamily.SansSerif,
        fontWeight = FontWeight.Normal,
        fontSize = 16.sp,
        lineHeight = 24.sp,
        letterSpacing = 0.5.sp
    ),
    bodyMedium = androidx.compose.ui.text.TextStyle(
        fontFamily = FontFamily.SansSerif,
        fontWeight = FontWeight.Normal,
        fontSize = 14.sp,
        lineHeight = 20.sp,
        letterSpacing = 0.25.sp
    ),
    bodySmall = androidx.compose.ui.text.TextStyle(
        fontFamily = FontFamily.SansSerif,
        fontWeight = FontWeight.Normal,
        fontSize = 12.sp,
        lineHeight = 16.sp,
        letterSpacing = 0.4.sp
    ),
    labelLarge = androidx.compose.ui.text.TextStyle(
        fontFamily = FontFamily.SansSerif,
        fontWeight = FontWeight.Medium,
        fontSize = 14.sp,
        lineHeight = 20.sp,
        letterSpacing = 0.1.sp
    ),
    labelMedium = androidx.compose.ui.text.TextStyle(
        fontFamily = FontFamily.SansSerif,
        fontWeight = FontWeight.Medium,
        fontSize = 12.sp,
        lineHeight = 16.sp,
        letterSpacing = 0.5.sp
    ),
    labelSmall = androidx.compose.ui.text.TextStyle(
        fontFamily = FontFamily.SansSerif,
        fontWeight = FontWeight.Medium,
        fontSize = 11.sp,
        lineHeight = 16.sp,
        letterSpacing = 0.5.sp
    )
)

@Composable
fun ApexRootTheme(
    darkTheme: Boolean = isSystemInDarkTheme(),
    content: @Composable () -> Unit
) {
    TextPrimary = if (darkTheme) Color(0xFFECECF5) else Color(0xFF1A1A2E)
    TextSecondary = if (darkTheme) Color(0xFF9A9AB0) else Color(0xFF6B6B80)
    TextTertiary = if (darkTheme) Color(0xFF5C5C78) else Color(0xFF9D9DB0)
    TextInverse = if (darkTheme) Color(0xFF1A1A2E) else Color(0xFFECECF5)

    CompositionLocalProvider(LocalIsDarkTheme provides darkTheme) {
        MaterialTheme(
            colorScheme = if (darkTheme) DarkColorScheme else LightColorScheme,
            typography = AppTypography,
            content = content
        )
    }
}
