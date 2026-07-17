package com.apex.root.ui.screens.scanresult

import androidx.compose.animation.animateContentSize
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material.icons.filled.CheckCircle
import androidx.compose.material.icons.filled.Error
import androidx.compose.material.icons.filled.ExpandLess
import androidx.compose.material.icons.filled.ExpandMore
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateMapOf
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.apex.root.viewmodel.trusted.ApexViewModel

/**
 * M3 ScanResultScreen — v1.0.5
 * 20 层检测结果,每层可展开/折叠,显示状态+详情。
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ScanResultScreen(
    onNavigateBack: () -> Unit,
    viewModel: ApexViewModel = viewModel()
) {
    val uiState by viewModel.uiState.collectAsState()
    val scrollBehavior = TopAppBarDefaults.exitUntilCollapsedScrollBehavior()
    val expandedStates = remember { mutableStateMapOf<Int, Boolean>() }

    Scaffold(
        modifier = Modifier.nestedScroll(scrollBehavior.nestedScrollConnection),
        topBar = {
            TopAppBar(
                title = { Text("检测结果") },
                navigationIcon = {
                    IconButton(onClick = onNavigateBack) {
                        Icon(Icons.Default.ArrowBack, contentDescription = "返回")
                    }
                },
                scrollBehavior = scrollBehavior
            )
        }
    ) { padding ->
        val layers = parseLayersFromScanResult(uiState.scanResult)

        LazyColumn(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding),
            contentPadding = androidx.compose.foundation.layout.PaddingValues(16.dp),
            verticalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            // 概览卡片
            item {
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    colors = CardDefaults.cardColors(
                        containerColor = MaterialTheme.colorScheme.primaryContainer
                    )
                ) {
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(20.dp),
                        horizontalArrangement = Arrangement.SpaceEvenly
                    ) {
                        Column(horizontalAlignment = Alignment.CenterHorizontally) {
                            Text(
                                "${uiState.layersPassed}",
                                style = MaterialTheme.typography.headlineMedium,
                                fontWeight = FontWeight.Bold,
                                color = MaterialTheme.colorScheme.secondary
                            )
                            Text("通过", style = MaterialTheme.typography.labelMedium)
                        }
                        Column(horizontalAlignment = Alignment.CenterHorizontally) {
                            Text(
                                "${uiState.layersTotal - uiState.layersPassed}",
                                style = MaterialTheme.typography.headlineMedium,
                                fontWeight = FontWeight.Bold,
                                color = MaterialTheme.colorScheme.error
                            )
                            Text("异常", style = MaterialTheme.typography.labelMedium)
                        }
                        Column(horizontalAlignment = Alignment.CenterHorizontally) {
                            Text(
                                "${uiState.riskScore}",
                                style = MaterialTheme.typography.headlineMedium,
                                fontWeight = FontWeight.Bold,
                                color = MaterialTheme.colorScheme.primary
                            )
                            Text("风险分", style = MaterialTheme.typography.labelMedium)
                        }
                    }
                }
            }

            // 层列表
            items(layers) { layer ->
                val isExpanded = expandedStates[layer.id] ?: false
                LayerCard(
                    layer = layer,
                    isExpanded = isExpanded,
                    onToggle = { expandedStates[layer.id] = !isExpanded }
                )
            }

            // 深度报告
            if (uiState.deepReport.isNotEmpty()) {
                item {
                    Card(modifier = Modifier.fillMaxWidth()) {
                        Column(modifier = Modifier.padding(16.dp)) {
                            Text("深度报告", style = MaterialTheme.typography.titleSmall)
                            Spacer(Modifier.height(8.dp))
                            Text(
                                uiState.deepReport.take(3000),
                                style = MaterialTheme.typography.bodySmall,
                                fontFamily = FontFamily.Monospace,
                                color = MaterialTheme.colorScheme.onSurfaceVariant
                            )
                        }
                    }
                }
            }
        }
    }
}

/**
 * 单层检测卡片,可展开/折叠
 */
@Composable
private fun LayerCard(
    layer: LayerInfo,
    isExpanded: Boolean,
    onToggle: () -> Unit
) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .animateContentSize(),
        onClick = onToggle,
        colors = CardDefaults.cardColors(
            containerColor = if (layer.detected)
                MaterialTheme.colorScheme.errorContainer.copy(alpha = 0.5f)
            else MaterialTheme.colorScheme.surface
        )
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            // 状态图标
            Icon(
                imageVector = if (layer.detected) Icons.Default.Error else Icons.Default.CheckCircle,
                contentDescription = null,
                tint = if (layer.detected) MaterialTheme.colorScheme.error
                    else MaterialTheme.colorScheme.secondary,
                modifier = Modifier.size(24.dp)
            )
            Spacer(Modifier.size(12.dp))

            // 层信息
            Column(modifier = Modifier.weight(1f)) {
                Text(
                    layer.name,
                    style = MaterialTheme.typography.bodyLarge,
                    fontWeight = FontWeight.Medium
                )
                Text(
                    if (layer.detected) "❌ 异常" else "✅ 正常",
                    style = MaterialTheme.typography.bodySmall,
                    color = if (layer.detected) MaterialTheme.colorScheme.error
                        else MaterialTheme.colorScheme.onSurfaceVariant
                )
            }

            // 展开图标
            Icon(
                imageVector = if (isExpanded) Icons.Default.ExpandLess else Icons.Default.ExpandMore,
                contentDescription = null,
                tint = MaterialTheme.colorScheme.onSurfaceVariant
            )
        }

        // 展开内容
        if (isExpanded && layer.detail.isNotEmpty()) {
            Spacer(Modifier.height(4.dp))
            Text(
                layer.detail,
                style = MaterialTheme.typography.bodySmall,
                fontFamily = FontFamily.Monospace,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
                modifier = Modifier.padding(horizontal = 16.dp, vertical = 8.dp)
            )
        }
    }
}

/**
 * 从扫描结果文本解析各层信息
 */
private data class LayerInfo(
    val id: Int,
    val name: String,
    val detected: Boolean,
    val detail: String
)

private fun parseLayersFromScanResult(result: String): List<LayerInfo> {
    val layers = mutableListOf<LayerInfo>()
    result.lines().forEach { line ->
        val trimmed = line.trim()
        // 匹配 "L1 系统属性:     ❌ 异常" 或 "L1 系统属性:     ✅ 正常"
        if (trimmed.startsWith("L") && trimmed.isNotEmpty()) {
            val afterL = trimmed.drop(1)
            if (afterL.isNotEmpty() && afterL.first().isDigit()) {
                val id = afterL.takeWhile { it.isDigit() }.toIntOrNull() ?: return@forEach
                val name = trimmed.substringBefore(":").trim()
                val detected = trimmed.contains("❌")
                val detail = if (detected) "检测到异常" else "未检测到异常"
                layers.add(LayerInfo(id, name, detected, detail))
            }
        }
    }
    return layers.sortedBy { it.id }
}
