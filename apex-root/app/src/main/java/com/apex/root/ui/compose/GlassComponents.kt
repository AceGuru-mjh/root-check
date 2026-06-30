package com.apex.root.ui.compose

import androidx.compose.animation.animateContentSize
import androidx.compose.animation.core.spring
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.KeyboardArrowDown
import androidx.compose.material.icons.filled.KeyboardArrowRight
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

@Composable
private fun hl(alpha: Float = 0.06f): Color =
    if (LocalIsDarkTheme.current) Color.White.copy(alpha = alpha) else Color.Black.copy(alpha = alpha)

@Composable
fun GlassCard(
    modifier: Modifier = Modifier,
    cornerRadius: Dp = 20.dp,
    accentLine: Color? = null,
    content: @Composable ColumnScope.() -> Unit
) {
    Card(
        modifier = modifier,
        shape = RoundedCornerShape(cornerRadius),
        colors = CardDefaults.cardColors(containerColor = Color.Transparent),
        elevation = CardDefaults.cardElevation(defaultElevation = 0.dp)
    ) {
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .clip(RoundedCornerShape(cornerRadius))
                .liquidGlass(cornerRadius = cornerRadius)
        ) {
            if (accentLine != null) {
                Box(
                    modifier = Modifier
                        .width(3.dp)
                        .height(32.dp)
                        .padding(start = 8.dp, top = 16.dp)
                        .background(
                            Brush.verticalGradient(listOf(accentLine, accentLine.copy(alpha = 0f))),
                            RoundedCornerShape(2.dp)
                        )
                        .align(Alignment.TopStart)
                )
            }
            Column(modifier = Modifier.padding(20.dp)) { content() }
        }
    }
}

@Composable
fun GlassFeatureCard(
    title: String,
    subtitle: String = "",
    icon: ImageVector,
    accentColor: Color,
    modifier: Modifier = Modifier,
    expandable: Boolean = false,
    expanded: Boolean = false,
    onToggle: (() -> Unit)? = null,
    content: @Composable ColumnScope.() -> Unit
) {
    val finalModifier = if (expandable) {
        modifier
            .animateContentSize(animationSpec = spring(dampingRatio = 0.75f, stiffness = 280f))
            .then(
                if (onToggle != null) Modifier.clickable(
                    interactionSource = remember { MutableInteractionSource() },
                    indication = null
                ) { onToggle() }
                else Modifier
            )
    } else modifier

    GlassCard(modifier = finalModifier, accentLine = accentColor) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            GlassIconBox(icon = icon, accentColor = accentColor, size = 38.dp)
            Spacer(Modifier.width(14.dp))
            Column(modifier = Modifier.weight(1f)) {
                Text(title, fontWeight = FontWeight.Bold, fontSize = 14.sp, color = TextPrimary,
                    letterSpacing = 0.3.sp)
                if (subtitle.isNotEmpty()) {
                    Spacer(Modifier.height(2.dp))
                    Text(subtitle, fontSize = 11.sp, color = TextSecondary)
                }
            }
            if (expandable) {
                Icon(
                    if (expanded) Icons.Default.KeyboardArrowDown else Icons.Default.KeyboardArrowRight,
                    contentDescription = null,
                    tint = TextTertiary,
                    modifier = Modifier.size(20.dp)
                )
            }
        }
        Spacer(Modifier.height(14.dp))
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .height(0.5.dp)
                .background(
                    Brush.horizontalGradient(
                        listOf(hl(0.06f), hl(0.02f), Color.Transparent)
                    )
                )
        )
        Spacer(Modifier.height(14.dp))
        content()
    }
}

@Composable
fun GlassExpandableCard(
    title: String,
    subtitle: String = "",
    icon: ImageVector,
    accentColor: Color,
    expandedContent: @Composable ColumnScope.() -> Unit,
    collapsedContent: @Composable ColumnScope.() -> Unit = {}
) {
    var isExpanded by remember { mutableStateOf(false) }

    GlassFeatureCard(
        title = title,
        subtitle = subtitle,
        icon = icon,
        accentColor = accentColor,
        expandable = true,
        expanded = isExpanded,
        onToggle = { isExpanded = !isExpanded }
    ) {
        collapsedContent()
        if (isExpanded) {
            Spacer(Modifier.height(8.dp))
            expandedContent()
        }
    }
}

@Composable
fun GlassIconBox(
    icon: ImageVector,
    accentColor: Color,
    size: Dp = 38.dp,
    iconSize: Dp = 20.dp
) {
    Box(
        modifier = Modifier
            .size(size)
            .clip(RoundedCornerShape(12.dp))
            .liquidGlassIcon(cornerRadius = 12.dp, baseColor = accentColor.copy(alpha = 0.12f)),
        contentAlignment = Alignment.Center
    ) {
        Icon(icon, contentDescription = null, modifier = Modifier.size(iconSize), tint = accentColor)
    }
}

@Composable
fun GlassGaugeScoreCard(
    score: Int,
    label: String,
    subtitle: String = "100 = 最安全"
) {
    val scoreColor = when {
        score > 60 -> ErrorRed
        score > 30 -> AccentGold
        score > 10 -> AccentMint
        else -> Color(0xFF00E676)
    }
    val progress = score / 100f

    GlassCard {
        Column(
            modifier = Modifier.fillMaxWidth(),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Box(contentAlignment = Alignment.Center, modifier = Modifier.size(200.dp)) {
                val track = hl(0.05f)
                Canvas(modifier = Modifier.size(170.dp)) {
                    drawArc(
                        color = track,
                        startAngle = 140f, sweepAngle = 260f,
                        useCenter = false,
                        style = Stroke(width = 16.dp.toPx(), cap = StrokeCap.Round)
                    )
                    drawArc(
                        brush = Brush.sweepGradient(
                            colors = listOf(scoreColor, AccentPurple, scoreColor),
                            center = Offset(size.width / 2f, size.height / 2f)
                        ),
                        startAngle = 140f, sweepAngle = 260f * progress,
                        useCenter = false,
                        style = Stroke(width = 16.dp.toPx(), cap = StrokeCap.Round)
                    )
                    drawArc(
                        color = scoreColor.copy(alpha = 0.08f),
                        startAngle = 138f,
                        sweepAngle = (260f * progress) + 4f,
                        useCenter = false,
                        style = Stroke(width = 22.dp.toPx(), cap = StrokeCap.Round)
                    )
                }
                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    Text("$score", fontSize = 36.sp, fontWeight = FontWeight.Bold,
                        color = scoreColor, letterSpacing = (-1).sp)
                    Text(label, fontSize = 13.sp, color = scoreColor.copy(alpha = 0.8f),
                        fontWeight = FontWeight.Medium)
                }
            }
            Spacer(Modifier.height(4.dp))
            Text(subtitle, fontSize = 10.sp, color = TextTertiary)
        }
    }
}

@Composable
fun GlassScoreCard(
    score: Int,
    label: String,
    subtitle: String = "100 = 最安全"
) {
    val scoreColor = when {
        score > 60 -> ErrorRed
        score > 30 -> AccentGold
        score > 10 -> AccentMint
        else -> Color(0xFF00E676)
    }

    GlassCard {
        Row(
            modifier = Modifier.fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Column {
                Text("安全评分", fontSize = 12.sp, color = TextSecondary,
                    fontWeight = FontWeight.Medium, letterSpacing = 0.5.sp)
                Spacer(Modifier.height(4.dp))
                Text(label, fontSize = 24.sp, fontWeight = FontWeight.Bold,
                    color = scoreColor, letterSpacing = 0.3.sp)
                Spacer(Modifier.height(4.dp))
                Text(subtitle, fontSize = 10.sp, color = TextTertiary)
            }
            Box(contentAlignment = Alignment.Center, modifier = Modifier.size(100.dp)) {
                val gaugeTrack = hl(0.05f)
                Canvas(modifier = Modifier.size(100.dp)) {
                    val sweep = (score / 100f) * 360f
                    val strokeW = 5.dp.toPx()
                    drawArc(
                        color = gaugeTrack,
                        startAngle = -90f, sweepAngle = 360f,
                        useCenter = false,
                        style = Stroke(width = strokeW, cap = StrokeCap.Round)
                    )
                    drawArc(
                        color = scoreColor,
                        startAngle = -90f, sweepAngle = sweep,
                        useCenter = false,
                        style = Stroke(width = strokeW, cap = StrokeCap.Round)
                    )
                    drawArc(
                        color = scoreColor.copy(alpha = 0.15f),
                        startAngle = -90f - 5f, sweepAngle = sweep + 10f,
                        useCenter = false,
                        style = Stroke(width = strokeW + 4.dp.toPx(), cap = StrokeCap.Round)
                    )
                }
                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    Text("$score", fontSize = 30.sp, fontWeight = FontWeight.Bold,
                        color = scoreColor, letterSpacing = (-0.5).sp)
                }
            }
        }
    }
}

@Composable
fun GlassFindingItem(label: String, value: String, color: Color) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 5.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Box(Modifier.size(5.dp).background(color, CircleShape))
        Spacer(Modifier.width(10.dp))
        Text(label, fontSize = 11.sp, color = TextTertiary, modifier = Modifier.weight(1f))
        Text(value, fontSize = 11.sp, color = color, fontWeight = FontWeight.Medium)
    }
}

@Composable
fun SectionHeader(title: String) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 20.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(title, fontSize = 12.sp, fontWeight = FontWeight.SemiBold,
            color = TextSecondary, letterSpacing = 0.5.sp)
        Spacer(Modifier.width(12.dp))
        Box(
            modifier = Modifier
                .weight(1f)
                .height(0.5.dp)
                .background(
                    Brush.horizontalGradient(listOf(hl(0.06f), Color.Transparent))
                )
        )
    }
}

@Composable
fun GlassSettingsGroup(
    title: String,
    content: @Composable () -> Unit
) {
    Column(modifier = Modifier.padding(horizontal = 20.dp)) {
        Text(
            text = title.uppercase(),
            color = TextTertiary,
            fontSize = 10.sp,
            letterSpacing = 1.sp,
            fontWeight = FontWeight.SemiBold,
            modifier = Modifier.padding(start = 8.dp, bottom = 10.dp)
        )
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .clip(RoundedCornerShape(20.dp))
                .liquidGlass(cornerRadius = 20.dp)
        ) {
            Column {
                content()
            }
        }
    }
}

@Composable
fun GlassSettingsItem(
    label: String,
    subtitle: String = "",
    icon: ImageVector? = null,
    accentColor: Color? = null,
    onClick: () -> Unit = {},
    trailing: @Composable (() -> Unit)? = null
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .clickable { onClick() }
            .padding(horizontal = 20.dp, vertical = 16.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            if (icon != null) {
                GlassIconBox(
                    icon = icon,
                    accentColor = accentColor ?: TextTertiary,
                    size = 32.dp, iconSize = 16.dp
                )
                Spacer(Modifier.width(14.dp))
            }
            Column {
                Text(label, color = TextPrimary, fontSize = 14.sp,
                    fontWeight = FontWeight.Medium)
                if (subtitle.isNotEmpty()) {
                    Text(subtitle, color = TextTertiary, fontSize = 11.sp)
                }
            }
        }
        if (trailing != null) {
            trailing()
        } else {
            Icon(
                Icons.Default.KeyboardArrowRight,
                contentDescription = null,
                tint = TextTertiary,
                modifier = Modifier.size(18.dp)
            )
        }
    }
    Divider(
        modifier = Modifier.padding(horizontal = 20.dp),
        color = hl(0.04f)
    )
}
