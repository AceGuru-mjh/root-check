package com.apex.root.ui.compose.screens

import androidx.compose.foundation.background

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.apex.root.ui.compose.*

data class BaselineMetric(
    val name: String,
    val baselineValue: String,
    val currentValue: String,
    val deviation: Double,
    val unit: String,
    val isAnomaly: Boolean
)

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun BaselineComparisonScreen(
    onBack: () -> Unit = {},
    metrics: List<BaselineMetric> = listOf(
        BaselineMetric("Syscall getpid", "110ns", "145ns", 1.32, "ns", true),
        BaselineMetric("Syscall open", "420ns", "430ns", 1.02, "ns", false),
        BaselineMetric("Cache 延迟", "80ns", "82ns", 1.03, "ns", false),
        BaselineMetric("Binder 时延", "140μs", "220μs", 1.57, "μs", true),
        BaselineMetric("中断时延", "580ns", "610ns", 1.05, "ns", false),
        BaselineMetric("IPC", "2.8", "2.1", 0.75, "", true)
    )
) {
    val scrollBehavior = TopAppBarDefaults.exitUntilCollapsedScrollBehavior()

    Scaffold(
        modifier = Modifier.nestedScroll(scrollBehavior.nestedScrollConnection),
        containerColor = Color.Transparent,
        topBar = {
            CollapsibleGlassTopBar(
                title = "对比基线",
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
            LazyColumn(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(padding)
                    .padding(16.dp),
                verticalArrangement = Arrangement.spacedBy(12.dp),
                contentPadding = PaddingValues(vertical = 8.dp)
            ) {
                item {
                    GlassCard(cornerRadius = 16.dp) {
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            GlassIconBox(icon = Icons.Default.Compare, accentColor = AccentGold, size = 36.dp, iconSize = 18.dp)
                            Spacer(Modifier.width(14.dp))
                            Column {
                                Text("纯净设备基准 vs 当前设备", fontWeight = FontWeight.SemiBold, fontSize = 14.sp, color = TextPrimary)
                                Text("${metrics.size} 项指标", fontSize = 11.sp, color = TextTertiary)
                            }
                        }
                    }
                }

                items(metrics) { metric ->
                    BaselineCard(metric)
                }

                item {
                    Spacer(Modifier.height(8.dp))
                    SectionHeader(title = "指标偏离度图表")
                    Spacer(Modifier.height(12.dp))
                    DeviationChart(metrics)
                    Spacer(Modifier.height(32.dp))
                }
            }
        }
    }
}

@Composable
private fun BaselineCard(metric: BaselineMetric) {
    val accentColor = if (metric.isAnomaly) ErrorRed else AccentMint

    GlassCard(cornerRadius = 16.dp, accentLine = accentColor) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically
        ) {
            GlassIconBox(
                icon = if (metric.isAnomaly) Icons.Default.Warning else Icons.Default.CheckCircle,
                accentColor = accentColor,
                size = 36.dp, iconSize = 18.dp
            )
            Spacer(Modifier.width(14.dp))
            Column(modifier = Modifier.weight(1f)) {
                Text(metric.name, fontWeight = FontWeight.Medium, fontSize = 13.sp, color = TextPrimary)
                Spacer(Modifier.height(4.dp))
                Text(
                    "基线: ${metric.baselineValue}${metric.unit}  当前: ${metric.currentValue}${metric.unit}",
                    fontFamily = FontFamily.Monospace,
                    fontSize = 12.sp,
                    color = TextSecondary
                )
                Text(
                    "偏离: ${"%.1f".format(metric.deviation)}x",
                    fontSize = 11.sp,
                    color = accentColor,
                    fontWeight = FontWeight.SemiBold
                )
            }
        }
    }
}

@Composable
private fun DeviationChart(metrics: List<BaselineMetric>) {
    GlassCard(cornerRadius = 16.dp) {
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .height(200.dp)
                .padding(16.dp)
        ) {
            Canvas(modifier = Modifier.fillMaxSize()) {
                val maxDev = metrics.maxOf { it.deviation }.coerceAtLeast(2.0)
                val barWidth = size.width / metrics.size * 0.55f
                val gap = size.width / metrics.size * 0.45f

                metrics.forEachIndexed { index, metric ->
                    val barHeight = (metric.deviation / maxDev * size.height * 0.75f).toFloat()
                    val x = index * (barWidth + gap) + gap / 2 + 8f
                    val color = if (metric.isAnomaly) ErrorRed else AccentMint

                    drawRoundRect(
                        color = color,
                        topLeft = Offset(x, size.height - barHeight),
                        size = androidx.compose.ui.geometry.Size(barWidth, barHeight),
                        cornerRadius = androidx.compose.ui.geometry.CornerRadius(4f, 4f)
                    )
                }
            }
        }
        Row(
            modifier = Modifier.fillMaxWidth().padding(horizontal = 20.dp, vertical = 8.dp),
            horizontalArrangement = Arrangement.Center
        ) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Box(Modifier.size(8.dp).background(ErrorRed, shape = RoundedCornerShape(2.dp)))
                Spacer(Modifier.width(6.dp))
                Text("异常", fontSize = 10.sp, color = TextTertiary)
            }
            Spacer(Modifier.width(20.dp))
            Row(verticalAlignment = Alignment.CenterVertically) {
                Box(Modifier.size(8.dp).background(AccentMint, shape = RoundedCornerShape(2.dp)))
                Spacer(Modifier.width(6.dp))
                Text("正常", fontSize = 10.sp, color = TextTertiary)
            }
        }
        Spacer(Modifier.height(8.dp))
    }
}
