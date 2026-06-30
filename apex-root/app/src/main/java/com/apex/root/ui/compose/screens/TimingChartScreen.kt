package com.apex.root.ui.compose.screens

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.apex.root.ui.compose.*
import kotlin.random.Random

data class TimingSeries(
    val name: String,
    val points: List<Long>,
    val color: Color
)

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun TimingChartScreen(
    onBack: () -> Unit = {},
    series: List<TimingSeries> = listOf(
        TimingSeries("基线(Baseline)", (1..50).map { 100L + (Random.nextLong() * 20).mod(20) }, AccentMint),
        TimingSeries("当前(Current)", (1..50).map { 100L + (Random.nextLong() * 80).mod(80) + (if (it > 30) 50L else 0L) }, ErrorRed)
    )
) {
    val scrollBehavior = TopAppBarDefaults.exitUntilCollapsedScrollBehavior()

    Scaffold(
        modifier = Modifier.nestedScroll(scrollBehavior.nestedScrollConnection),
        containerColor = Color.Transparent,
        topBar = {
            CollapsibleGlassTopBar(
                title = "侧信道计时可视化",
                collapsedFraction = scrollBehavior.state.collapsedFraction,
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.Default.ArrowBack, "返回")
                    }
                }
            )
        }
    ) { padding ->
        LiquidGlassContainer(
            fluidColorsDark = PageFluidColors.report,
            fluidColorsLight = PageFluidColors.reportLight
        ) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(padding)
                    .padding(16.dp)
                    .verticalScroll(rememberScrollState()),
                verticalArrangement = Arrangement.spacedBy(16.dp)
            ) {
                GlassCard(cornerRadius = 16.dp) {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        GlassIconBox(icon = Icons.Default.Timeline, accentColor = AccentGold, size = 36.dp, iconSize = 18.dp)
                        Spacer(Modifier.width(14.dp))
                        Text("Syscall 耗时曲线 - APatch/KSU 时延异常分析", fontSize = 12.sp, color = TextSecondary)
                    }
                }

                GlassCard(cornerRadius = 16.dp) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceEvenly
                    ) {
                        series.forEach { s ->
                            Row(verticalAlignment = Alignment.CenterVertically) {
                                Box(Modifier.size(10.dp).background(s.color, CircleShape))
                                Spacer(Modifier.width(6.dp))
                                Text(s.name, fontSize = 11.sp, color = TextSecondary)
                            }
                        }
                    }
                    Spacer(Modifier.height(12.dp))
                    Box(modifier = Modifier.fillMaxWidth().height(260.dp)) {
                        TimingCanvas(series = series, modifier = Modifier.fillMaxSize())
                    }
                }

                series.forEach { s ->
                    val avg = s.points.average().toLong()
                    val max = s.points.max()
                    val min = s.points.min()

                    GlassCard(cornerRadius = 16.dp, accentLine = s.color) {
                        Text(s.name, fontWeight = FontWeight.SemiBold, fontSize = 14.sp, color = TextPrimary)
                        Spacer(Modifier.height(8.dp))
                        Column {
                            StatRow("最小", "$min ns", s.color)
                            StatRow("最大", "$max ns", s.color)
                            StatRow("平均", "$avg ns", s.color)
                            StatRow("样本数", "${s.points.size}", TextTertiary)
                        }
                    }
                }

                if (series.size >= 2) {
                    val avg1 = series[0].points.average()
                    val avg2 = series[1].points.average()
                    val deviation = if (avg1 > 0) avg2 / avg1 else 0.0
                    val isAnomaly = deviation > 1.3

                    GlassCard(cornerRadius = 16.dp, accentLine = if (isAnomaly) ErrorRed else AccentMint) {
                        Text("时延偏移分析", fontWeight = FontWeight.SemiBold, fontSize = 14.sp, color = TextPrimary)
                        Spacer(Modifier.height(10.dp))
                        Column {
                            Row(verticalAlignment = Alignment.CenterVertically) {
                                Text("偏离度: ", fontSize = 12.sp, color = TextSecondary)
                                Text(
                                    "${"%.2f".format(deviation)}x",
                                    fontFamily = FontFamily.Monospace,
                                    fontSize = 16.sp,
                                    fontWeight = FontWeight.Bold,
                                    color = if (isAnomaly) ErrorRed else AccentMint
                                )
                            }
                            Spacer(Modifier.height(6.dp))
                            Row(verticalAlignment = Alignment.CenterVertically) {
                                Icon(
                                    if (isAnomaly) Icons.Default.Warning else Icons.Default.CheckCircle,
                                    null,
                                    tint = if (isAnomaly) ErrorRed else AccentMint,
                                    modifier = Modifier.size(16.dp)
                                )
                                Spacer(Modifier.width(6.dp))
                                Text(
                                    if (isAnomaly) "检测到显著时延偏移，可能存在内核 Hook" else "时延在正常范围内",
                                    fontSize = 12.sp,
                                    color = if (isAnomaly) ErrorRed else AccentMint
                                )
                            }
                        }
                    }
                }

                Spacer(Modifier.height(32.dp))
            }
        }
    }
}

@Composable
private fun StatRow(label: String, value: String, color: Color) {
    Row(
        modifier = Modifier.fillMaxWidth().padding(vertical = 2.dp),
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(label, fontSize = 11.sp, color = TextTertiary)
        Text(value, fontFamily = FontFamily.Monospace, fontSize = 12.sp, color = color, fontWeight = FontWeight.Medium)
    }
}

@Composable
private fun TimingCanvas(
    series: List<TimingSeries>,
    modifier: Modifier = Modifier
) {
    Canvas(modifier = modifier.padding(12.dp)) {
        if (series.isEmpty()) return@Canvas

        val maxVal = series.flatMap { it.points }.max().toFloat().coerceAtLeast(1f)
        val padding = 40f
        val chartWidth = size.width - padding * 2
        val chartHeight = size.height - padding * 2

        series.forEach { s ->
            if (s.points.isEmpty()) return@forEach
            val stepX = chartWidth / (s.points.size - 1).coerceAtLeast(1)

            val path = Path()
            s.points.forEachIndexed { index, value ->
                val x = padding + index * stepX
                val y = size.height - padding - (value.toFloat() / maxVal * chartHeight)
                if (index == 0) path.moveTo(x, y) else path.lineTo(x, y)
            }
            drawPath(path, s.color, style = Stroke(width = 2.5f))
        }
    }
}
