package com.rootdetector.ui.compose.components

import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.rootdetector.domain.model.ThreatLevel
import com.rootdetector.ui.model.threatLevelLabel
import com.rootdetector.ui.compose.theme.CriticalPurple
import com.rootdetector.ui.compose.theme.HighRed
import com.rootdetector.ui.compose.theme.MediumOrange
import com.rootdetector.ui.compose.theme.LowAmber
import com.rootdetector.ui.compose.theme.SafeGreen

@Composable
fun ThreatGauge(
    riskScore: Int,
    threatLevel: ThreatLevel,
    zh: Boolean = true,
    modifier: Modifier = Modifier,
    size: Dp = 200.dp,
    strokeWidth: Dp = 16.dp
) {
    val animatedProgress by animateFloatAsState(
        targetValue = riskScore / 100f,
        animationSpec = tween(1000),
        label = "gaugeProgress"
    )

    val gaugeColor = when (threatLevel) {
        ThreatLevel.SAFE -> SafeGreen
        ThreatLevel.LOW -> LowAmber
        ThreatLevel.MEDIUM -> MediumOrange
        ThreatLevel.HIGH -> HighRed
        ThreatLevel.CRITICAL -> CriticalPurple
    }

    val trackColor = Color.Gray.copy(alpha = 0.2f)
    val sweepAngle = 270f
    val startAngle = 135f

    Box(
        contentAlignment = Alignment.Center,
        modifier = modifier
            .size(size)
            .padding(8.dp)
    ) {
        Canvas(modifier = Modifier.size(size)) {
            val stroke = Stroke(width = strokeWidth.toPx(), cap = StrokeCap.Round)
            val arcSize = Size(size.toPx() - strokeWidth.toPx(), size.toPx() - strokeWidth.toPx())
            val arcTopLeft = Offset(strokeWidth.toPx() / 2, strokeWidth.toPx() / 2)

            drawArc(
                color = trackColor,
                startAngle = startAngle,
                sweepAngle = sweepAngle,
                useCenter = false,
                topLeft = arcTopLeft,
                size = arcSize,
                style = stroke
            )

            drawArc(
                color = gaugeColor,
                startAngle = startAngle,
                sweepAngle = sweepAngle * animatedProgress,
                useCenter = false,
                topLeft = arcTopLeft,
                size = arcSize,
                style = stroke
            )
        }

        Column(horizontalAlignment = Alignment.CenterHorizontally) {
            Text(
                text = "$riskScore",
                fontSize = 36.sp,
                fontWeight = FontWeight.Bold,
                color = gaugeColor
            )
            Text(
                text = threatLevelLabel(threatLevel, zh),
                fontSize = 14.sp,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
        }
    }
}
