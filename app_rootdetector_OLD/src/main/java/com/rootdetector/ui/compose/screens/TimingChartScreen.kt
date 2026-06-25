package com.rootdetector.ui.compose.screens

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

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
        TimingSeries("基线(Baseline)", (1..50).map { 100 + (Math.random() * 20).toLong() }, Color(0xFF43A047)),
        TimingSeries("当前(Current)", (1..50).map { 100 + (Math.random() * 80).toLong() + (if (it > 30) 50 else 0) }, Color(0xFFE53935))
    )
) {
    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("侧信道计时可视化") },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = null)
                    }
                }
            )
        }
    ) { padding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
                .padding(16.dp)
                .verticalScroll(rememberScrollState()),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            Text(
                "Syscall 耗时曲线 — 直观展示 APatch/KSU 时延异常",
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )

            Card(
                modifier = Modifier.fillMaxWidth().height(280.dp),
                shape = RoundedCornerShape(12.dp),
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant.copy(alpha = 0.4f))
            ) {
                Column(modifier = Modifier.padding(12.dp)) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceEvenly
                    ) {
                        series.forEach { s ->
                            Row {
                                Canvas(modifier = Modifier.size(12.dp)) {
                                    drawCircle(s.color)
                                }
                                Spacer(Modifier.width(4.dp))
                                Text(s.name, fontSize = 12.sp)
                            }
                        }
                    }
                    Spacer(Modifier.height(8.dp))
                    TimingCanvas(series = series, modifier = Modifier.fillMaxSize())
                }
            }

            series.forEach { s ->
                val avg = s.points.average().toLong()
                val max = s.points.max()
                val min = s.points.min()
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    shape = RoundedCornerShape(12.dp),
                    colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant.copy(alpha = 0.4f))
                ) {
                    Column(modifier = Modifier.padding(16.dp)) {
                        Text(s.name, fontWeight = FontWeight.SemiBold)
                        Spacer(Modifier.height(4.dp))
                        Text("最小: $min ns", fontFamily = FontFamily.Monospace, fontSize = 13.sp)
                        Text("最大: $max ns", fontFamily = FontFamily.Monospace, fontSize = 13.sp)
                        Text("平均: $avg ns", fontFamily = FontFamily.Monospace, fontSize = 13.sp)
                        Text("样本数: ${s.points.size}", fontFamily = FontFamily.Monospace, fontSize = 13.sp)
                    }
                }
            }

            if (series.size >= 2) {
                val avg1 = series[0].points.average()
                val avg2 = series[1].points.average()
                val deviation = if (avg1 > 0) avg2 / avg1 else 0.0
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    shape = RoundedCornerShape(12.dp),
                    colors = CardDefaults.cardColors(
                        containerColor = if (deviation > 1.3) MaterialTheme.colorScheme.errorContainer.copy(alpha = 0.3f)
                        else MaterialTheme.colorScheme.surfaceVariant.copy(alpha = 0.4f)
                    )
                ) {
                    Column(modifier = Modifier.padding(16.dp)) {
                        Text("时延偏移分析", fontWeight = FontWeight.SemiBold)
                        Text(
                            "偏离度: ${"%.2f".format(deviation)}x",
                            fontFamily = FontFamily.Monospace,
                            fontSize = 13.sp,
                            color = if (deviation > 1.3) MaterialTheme.colorScheme.error
                            else MaterialTheme.colorScheme.onSurfaceVariant
                        )
                        Text(
                            if (deviation > 1.3) "⚠️ 检测到显著时延偏移，可能存在内核 Hook"
                            else "✅ 时延在正常范围内",
                            fontSize = 13.sp
                        )
                    }
                }
            }

            Spacer(Modifier.height(32.dp))
        }
    }
}

@Composable
private fun TimingCanvas(
    series: List<TimingSeries>,
    modifier: Modifier = Modifier
) {
    Canvas(modifier = modifier) {
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
            drawPath(path, s.color, style = Stroke(width = 2f))
        }
    }
}
