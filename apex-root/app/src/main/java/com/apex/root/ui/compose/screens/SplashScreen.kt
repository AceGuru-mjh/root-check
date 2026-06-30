package com.apex.root.ui.compose.screens

import android.os.Build
import android.graphics.ColorMatrix
import android.graphics.ColorMatrixColorFilter
import android.graphics.RenderEffect
import android.graphics.Shader
import androidx.compose.animation.core.*
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.blur
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.asComposeRenderEffect
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.apex.root.ui.compose.*
import kotlinx.coroutines.delay
import kotlin.math.cos
import kotlin.math.sin

@Composable
fun SplashScreen(onSplashComplete: () -> Unit) {
    var visible by remember { mutableStateOf(true) }

    LaunchedEffect(Unit) {
        delay(2400)
        visible = false
        onSplashComplete()
    }

    if (!visible) return

    // 使用屏幕实际尺寸自适应
    val configuration = LocalConfiguration.current
    val screenWidthDp = configuration.screenWidthDp.dp
    val screenHeightDp = configuration.screenHeightDp.dp

    val transition = rememberInfiniteTransition()
    val animTime by transition.animateFloat(
        initialValue = 0f,
        targetValue = (2f * Math.PI).toFloat(),
        animationSpec = infiniteRepeatable(tween(3500, easing = LinearEasing))
    )

    Box(
        modifier = Modifier.fillMaxSize().background(DeepBackground),
        contentAlignment = Alignment.Center
    ) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            MetaballEffect(animTime, screenWidthDp, screenHeightDp)
        } else {
            FallbackMetaball(animTime, screenWidthDp, screenHeightDp)
        }

        Column(
            modifier = Modifier.align(Alignment.Center),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            // 修复：原 fillMaxHeight(0.25f) 在 Box.align(Center) 内会推下内容到屏幕外
            // 改为固定 Spacer，让 CircularProgressIndicator 在小屏上也能显示
            Spacer(Modifier.height(60.dp))
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text("APEX", color = Color.White,
                    style = MaterialTheme.typography.headlineLarge,
                    fontWeight = FontWeight.Bold, letterSpacing = 2.sp)
                Text(" Root", color = Color.White.copy(alpha = 0.5f),
                    style = MaterialTheme.typography.headlineLarge,
                    fontWeight = FontWeight.Light, letterSpacing = 2.sp)
            }
            Spacer(Modifier.height(8.dp))
            Text("全能安全平台", color = TextTertiary, fontSize = 12.sp, letterSpacing = 3.sp)
            Spacer(Modifier.height(36.dp))
            CircularProgressIndicator(
                modifier = Modifier.size(18.dp),
                strokeWidth = 1.5.dp,
                color = AccentPurple.copy(alpha = 0.5f)
            )
        }
    }
}

@androidx.annotation.RequiresApi(Build.VERSION_CODES.S)
@Composable
private fun MetaballEffect(animTime: Float, screenWidthDp: androidx.compose.ui.unit.Dp, screenHeightDp: androidx.compose.ui.unit.Dp) {
    val alphaMatrix = remember {
        ColorMatrix().apply {
            set(floatArrayOf(
                1f, 0f, 0f, 0f, 0f,
                0f, 1f, 0f, 0f, 0f,
                0f, 0f, 1f, 0f, 0f,
                0f, 0f, 0f, 40f, -2500f
            ))
        }
    }

    // 自适应尺寸：取屏幕宽高的 80%，不超过 360dp
    val effectSize = minOf(screenWidthDp, screenHeightDp) * 0.8f

    Box(
        modifier = Modifier
            .size(effectSize)
            .graphicsLayer {
                val blur = RenderEffect.createBlurEffect(40f, 40f, Shader.TileMode.CLAMP)
                val matrixFilter = RenderEffect.createColorFilterEffect(
                    ColorMatrixColorFilter(alphaMatrix)
                )
                renderEffect = RenderEffect.createChainEffect(matrixFilter, blur).asComposeRenderEffect()
            },
        contentAlignment = Alignment.Center
    ) {
        Box(
            Modifier.size(80.dp)
                .background(Color(0xFF00F2FE), CircleShape)
                .align(Alignment.Center)
        )
        Box(
            Modifier
                .size(46.dp)
                .align(Alignment.Center)
                .graphicsLayer {
                    translationX = sin(animTime) * 100.dp.toPx()
                    translationY = cos(animTime) * 100.dp.toPx()
                }
                .background(Color(0xFF4FACFE), CircleShape)
        )
        Box(
            Modifier
                .size(38.dp)
                .align(Alignment.Center)
                .graphicsLayer {
                    translationX = sin(animTime + 3f) * 80.dp.toPx()
                    translationY = cos(animTime + 3f) * 80.dp.toPx()
                }
                .background(AccentMintSoft, CircleShape)
        )
    }
}

@Composable
private fun FallbackMetaball(animTime: Float, screenWidthDp: androidx.compose.ui.unit.Dp, screenHeightDp: androidx.compose.ui.unit.Dp) {
    // 自适应尺寸
    val effectSize = minOf(screenWidthDp, screenHeightDp) * 0.8f

    Box(
        modifier = Modifier.size(effectSize),
        contentAlignment = Alignment.Center
    ) {
        Box(
            Modifier.size(80.dp)
                .blur(20.dp)
                .background(Color(0xFF00F2FE).copy(alpha = 0.4f), CircleShape)
                .align(Alignment.Center)
        )
        Box(
            Modifier
                .size(46.dp)
                .offset(x = (sin(animTime) * 100).dp, y = (cos(animTime) * 100).dp)
                .blur(16.dp)
                .background(Color(0xFF4FACFE).copy(alpha = 0.35f), CircleShape)
        )
        Box(
            Modifier
                .size(38.dp)
                .offset(x = (sin(animTime + 3f) * 80).dp, y = (cos(animTime + 3f) * 80).dp)
                .blur(14.dp)
                .background(AccentMintSoft.copy(alpha = 0.3f), CircleShape)
        )
    }
}
