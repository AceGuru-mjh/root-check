package com.apex.root.ui.screens.dashboard

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material.icons.filled.Shield
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.apex.root.viewmodel.trusted.ApexViewModel

/**
 * M3 Dashboard — v1.0.4
 * 使用现有 ApexViewModel, LazyColumn 滚动, M3 组件。
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun DashboardScreen(
    onNavigateToSettings: () -> Unit,
    viewModel: ApexViewModel = viewModel()
) {
    val uiState by viewModel.uiState.collectAsState()
    val scrollBehavior = TopAppBarDefaults.exitUntilCollapsedScrollBehavior()

    Scaffold(
        modifier = Modifier.nestedScroll(scrollBehavior.nestedScrollConnection),
        topBar = {
            TopAppBar(
                title = { Text("Apex Agent") },
                actions = {
                    IconButton(onClick = onNavigateToSettings) {
                        Icon(Icons.Default.Settings, contentDescription = "设置")
                    }
                },
                scrollBehavior = scrollBehavior
            )
        }
    ) { padding ->
        LazyColumn(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding),
            contentPadding = androidx.compose.foundation.layout.PaddingValues(16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            // 风险评分卡片
            item {
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    colors = CardDefaults.cardColors(
                        containerColor = MaterialTheme.colorScheme.primaryContainer
                    )
                ) {
                    Column(modifier = Modifier.padding(20.dp)) {
                        Text("风险评分", style = MaterialTheme.typography.labelLarge)
                        Spacer(Modifier.height(8.dp))
                        Text(
                            "${uiState.riskScore}/100",
                            style = MaterialTheme.typography.displayMedium,
                            fontWeight = FontWeight.Bold,
                            color = MaterialTheme.colorScheme.onPrimaryContainer
                        )
                        Spacer(Modifier.height(4.dp))
                        val riskLabel = when {
                            uiState.riskScore > 60 -> "高风险"
                            uiState.riskScore > 30 -> "中等风险"
                            uiState.riskScore > 10 -> "轻度风险"
                            else -> "安全"
                        }
                        Text(riskLabel, style = MaterialTheme.typography.bodyMedium)
                    }
                }
            }

            // 扫描进度
            if (uiState.isScanning) {
                item {
                    Card(modifier = Modifier.fillMaxWidth()) {
                        Column(modifier = Modifier.padding(16.dp)) {
                            Text(
                                "扫描中... ${uiState.currentLayer}",
                                style = MaterialTheme.typography.bodyMedium
                            )
                            Spacer(Modifier.height(8.dp))
                            LinearProgressIndicator(
                                progress = uiState.scanProgress,
                                modifier = Modifier.fillMaxWidth()
                            )
                        }
                    }
                }
            }

            // 扫描按钮
            item {
                Button(
                    onClick = { viewModel.runScan() },
                    enabled = !uiState.isScanning,
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Icon(Icons.Default.Shield, contentDescription = null)
                    Spacer(Modifier.height(0.dp))
                    Text(
                        if (uiState.isScanning) "扫描中..." else "快速检测",
                        modifier = Modifier.padding(start = 8.dp)
                    )
                }
            }

            // 并行扫描按钮
            item {
                Button(
                    onClick = { viewModel.runParallelScan() },
                    enabled = !uiState.isScanning,
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text("并行扫描 (19层并发)")
                }
            }

            // 深度扫描按钮
            item {
                androidx.compose.material3.OutlinedButton(
                    onClick = { viewModel.runDeepScan() },
                    enabled = !uiState.isScanning,
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text("深度检测")
                }
            }

            // 检测结果
            if (uiState.scanResult.isNotEmpty() && uiState.scanResult != "点击扫描开始检测") {
                item {
                    Card(modifier = Modifier.fillMaxWidth()) {
                        Column(modifier = Modifier.padding(16.dp)) {
                            Text("检测结果", style = MaterialTheme.typography.titleMedium)
                            Spacer(Modifier.height(8.dp))
                            Text(
                                uiState.scanResult.take(2000),
                                style = MaterialTheme.typography.bodySmall,
                                fontFamily = androidx.compose.ui.text.font.FontFamily.Monospace
                            )
                        }
                    }
                }
            }

            // 层通过数
            if (uiState.layersTotal > 0) {
                item {
                    Card(modifier = Modifier.fillMaxWidth()) {
                        Text(
                            "检测层: ${uiState.layersPassed}/${uiState.layersTotal} 通过",
                            modifier = Modifier.padding(16.dp),
                            style = MaterialTheme.typography.bodyMedium
                        )
                    }
                }
            }

            // Native 库状态
            item {
                Card(modifier = Modifier.fillMaxWidth()) {
                    Text(
                        "Native 引擎: ${if (uiState.nativeAvailable) "✅ 已加载" else "❌ 未加载"}",
                        modifier = Modifier.padding(16.dp),
                        style = MaterialTheme.typography.bodyMedium
                    )
                }
            }
        }
    }
}
