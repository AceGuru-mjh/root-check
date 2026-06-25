package com.rootdetector.ui.compose.screens

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.CheckCircle
import androidx.compose.material.icons.filled.Warning
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

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
    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("对比基线") },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = null)
                    }
                }
            )
        }
    ) { padding ->
        LazyColumn(
            modifier = Modifier.fillMaxSize().padding(padding).padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            item {
                Text(
                    "纯净设备基准与当前设备对比",
                    style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
                Spacer(Modifier.height(8.dp))
            }

            items(metrics) { metric ->
                BaselineCard(metric)
            }

            item {
                Spacer(Modifier.height(16.dp))
                Text(
                    "图表：各指标偏离度",
                    style = MaterialTheme.typography.titleSmall,
                    fontWeight = FontWeight.SemiBold
                )
                Spacer(Modifier.height(8.dp))
                DeviationChart(metrics)
                Spacer(Modifier.height(32.dp))
            }
        }
    }
}

@Composable
private fun BaselineCard(metric: BaselineMetric) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        shape = RoundedCornerShape(12.dp),
        colors = CardDefaults.cardColors(
            containerColor = if (metric.isAnomaly)
                MaterialTheme.colorScheme.errorContainer.copy(alpha = 0.3f)
            else MaterialTheme.colorScheme.surfaceVariant.copy(alpha = 0.4f)
        )
    ) {
        Row(
            modifier = Modifier.padding(16.dp).fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Icon(
                if (metric.isAnomaly) Icons.Default.Warning else Icons.Default.CheckCircle,
                contentDescription = null,
                tint = if (metric.isAnomaly) MaterialTheme.colorScheme.error else MaterialTheme.colorScheme.primary,
                modifier = Modifier.size(24.dp)
            )
            Spacer(Modifier.width(12.dp))
            Column(modifier = Modifier.weight(1f)) {
                Text(metric.name, fontWeight = FontWeight.Medium)
                Spacer(Modifier.height(4.dp))
                Text(
                    "基线: ${metric.baselineValue}${metric.unit}  当前: ${metric.currentValue}${metric.unit}",
                    fontFamily = FontFamily.Monospace,
                    fontSize = 13.sp
                )
                Text(
                    "偏离: ${"%.1f".format(metric.deviation)}x",
                    fontSize = 12.sp,
                    color = if (metric.isAnomaly) MaterialTheme.colorScheme.error
                    else MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        }
    }
}

@Composable
private fun DeviationChart(metrics: List<BaselineMetric>) {
    Card(
        modifier = Modifier.fillMaxWidth().height(200.dp),
        shape = RoundedCornerShape(12.dp),
        colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant.copy(alpha = 0.4f))
    ) {
        Canvas(modifier = Modifier.fillMaxSize().padding(24.dp)) {
            val maxDev = metrics.maxOf { it.deviation }.coerceAtLeast(2.0)
            val barWidth = size.width / metrics.size * 0.6f
            val gap = size.width / metrics.size * 0.4f

            metrics.forEachIndexed { index, metric ->
                val barHeight = (metric.deviation / maxDev * size.height * 0.8f).toFloat()
                val x = index * (barWidth + gap) + gap / 2
                val color = if (metric.isAnomaly) Color(0xFFE53935) else Color(0xFF43A047)

                drawRect(
                    color = color,
                    topLeft = Offset(x, size.height - barHeight),
                    size = androidx.compose.ui.geometry.Size(barWidth, barHeight)
                )
            }
        }
    }
}
