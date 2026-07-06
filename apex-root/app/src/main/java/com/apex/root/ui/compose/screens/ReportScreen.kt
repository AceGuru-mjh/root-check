package com.apex.root.ui.compose.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.apex.root.domain.trust.model.Finding
import com.apex.root.domain.trust.model.GlobalSecureReport
import com.apex.root.domain.trust.model.Severity
import com.apex.root.ui.compose.*
import com.apex.root.viewmodel.trusted.ScanViewModel
import com.apex.root.viewmodel.trusted.UiState

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ReportScreen(
    viewModel: ScanViewModel,
    onBack: () -> Unit
) {
    val uiState by viewModel.uiState.collectAsState()
    val report = (uiState as? UiState.Report)?.report
    val isScanning = uiState is UiState.Scanning || uiState is UiState.Connecting
    val context = LocalContext.current

    Scaffold(
        containerColor = Color.Transparent,
        topBar = {
            TopAppBar(
                title = { Text("安全报告", fontWeight = FontWeight.Bold, letterSpacing = 0.5.sp) },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.Default.ArrowBack, "返回")
                    }
                },
                actions = {
                    // 新增：分享报告按钮
                    if (report != null) {
                        IconButton(onClick = {
                            val reportText = formatReportAsText(report)
                            val sendIntent = android.content.Intent(android.content.Intent.ACTION_SEND).apply {
                                type = "text/plain"
                                putExtra(android.content.Intent.EXTRA_SUBJECT, "APEX-Root 安全报告")
                                putExtra(android.content.Intent.EXTRA_TEXT, reportText)
                            }
                            runCatching {
                                context.startActivity(android.content.Intent.createChooser(sendIntent, "分享报告"))
                            }
                        }) {
                            Icon(Icons.Default.Share, "分享报告")
                        }
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(containerColor = Color.Transparent)
            )
        }
    ) { padding ->
        LiquidGlassContainer(fluidColorsDark = PageFluidColors.report, fluidColorsLight = PageFluidColors.reportLight) {
            if (isScanning) {
                Box(modifier = Modifier.fillMaxSize().padding(padding), contentAlignment = Alignment.Center) {
                    Column(horizontalAlignment = Alignment.CenterHorizontally) {
                        CircularProgressIndicator(modifier = Modifier.size(36.dp), strokeWidth = 3.dp, color = AccentBlue)
                        Spacer(Modifier.height(14.dp))
                        Text("正在生成安全报告...", fontSize = 13.sp, color = TextSecondary)
                    }
                }
            } else if (report == null) {
                Box(modifier = Modifier.fillMaxSize().padding(padding), contentAlignment = Alignment.Center) {
                    GlassEmptyState(
                        title = "暂无报告数据",
                        description = "运行扫描以生成安全报告",
                        icon = {
                            Box(Modifier.size(80.dp).background(if (LocalIsDarkTheme.current) AccentBlue.copy(alpha = 0.15f) else PastelBlue, CircleShape), contentAlignment = Alignment.Center) {
                                Icon(Icons.Default.Description, null, Modifier.size(40.dp), tint = AccentBlue)
                            }
                        },
                        modifier = Modifier.padding(16.dp)
                    )
                }
            } else {
                LazyColumn(
                    modifier = Modifier
                        .fillMaxSize()
                        .padding(padding)
                        .padding(16.dp),
                    verticalArrangement = Arrangement.spacedBy(14.dp)
                ) {
                    item { ReportSummaryCard(report) }
                    items(report.results) { result ->
                        LayerResultCard(
                            serviceName = result.serviceName,
                            serviceId = result.serviceId,
                            success = result.success,
                            confidence = result.confidence,
                            findings = result.findings,
                            durationMs = result.durationMs
                        )
                    }
                }
            }
        }
    }
}

@Composable
private fun ReportSummaryCard(report: GlobalSecureReport) {
    val riskColor = when (report.overallRisk) {
        Severity.CRITICAL -> ErrorRed
        Severity.HIGH -> Color(0xFFFF7043)
        Severity.MEDIUM -> AccentGold
        Severity.LOW -> AccentMint
        else -> AccentMint
    }

    // 新增：计算层通过数
    val passedCount = report.results.count { it.success }
    val totalCount = report.results.size

    GlassCard {
        Row(
            modifier = Modifier.fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Box(
                modifier = Modifier
                    .size(56.dp)
                    .clip(CircleShape)
                    .liquidGlassIcon(28.dp, riskColor.copy(alpha = 0.15f)),
                contentAlignment = Alignment.Center
            ) {
                Icon(Icons.Default.Shield, null, Modifier.size(28.dp), tint = riskColor)
            }
            Spacer(Modifier.width(16.dp))
            Column {
                Text("总体风险评估", fontSize = 12.sp, color = TextTertiary, letterSpacing = 0.3.sp)
                Spacer(Modifier.height(2.dp))
                Text(report.overallRisk.name, fontSize = 20.sp, fontWeight = FontWeight.Bold,
                    color = riskColor, letterSpacing = 0.3.sp)
                Text("风险评分: ${"%.1f".format(report.riskScore)}%",
                    fontSize = 12.sp, color = TextSecondary)
                // 新增：层通过数总结
                if (totalCount > 0) {
                    Spacer(Modifier.height(4.dp))
                    Text(
                        "$passedCount / $totalCount 层通过",
                        fontSize = 11.sp,
                        color = if (passedCount == totalCount) AccentMint else AccentGold,
                        fontWeight = FontWeight.Medium
                    )
                }
            }
        }
    }
}

@Composable
private fun LayerResultCard(
    serviceName: String,
    serviceId: String,
    success: Boolean,
    confidence: Float,
    findings: List<Finding>,
    durationMs: Long
) {
    var expanded by remember { mutableStateOf(false) }
    val statusColor = if (success) AccentMint else ErrorRed

    GlassCard(
        modifier = Modifier.clickable { expanded = !expanded },
        accentLine = statusColor
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically
        ) {
            GlassIconBox(
                icon = if (success) Icons.Default.CheckCircle else Icons.Default.Warning,
                accentColor = statusColor,
                size = 32.dp, iconSize = 18.dp
            )
            Spacer(Modifier.width(12.dp))
            Column(modifier = Modifier.weight(1f)) {
                Text(serviceName, fontWeight = FontWeight.Bold, fontSize = 13.sp)
                Text(serviceId, fontSize = 10.sp, color = TextTertiary)
            }
            Column(horizontalAlignment = Alignment.End) {
                Text(if (success) "通过" else "异常",
                    color = statusColor, fontWeight = FontWeight.Bold, fontSize = 11.sp)
                Text("${(confidence * 100).toInt()}%", fontSize = 9.sp, color = TextTertiary)
            }
        }

        if (expanded) {
            Spacer(Modifier.height(14.dp))
            Box(
                Modifier.fillMaxWidth().height(0.5.dp)
                    .background(Color.White.copy(alpha = 0.04f))
            )
            Spacer(Modifier.height(14.dp))
            findings.forEach { finding ->
                ReportFindingItem(finding)
                Spacer(Modifier.height(8.dp))
            }
        }

        Spacer(Modifier.height(8.dp))
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text("${durationMs}ms", fontSize = 10.sp, color = TextTertiary)
            Text(if (expanded) "收起" else "查看详情",
                fontSize = 10.sp, color = AccentPurple)
        }
    }
}

@Composable
private fun ReportFindingItem(finding: Finding) {
    val sevColor = when (finding.severity) {
        Severity.CRITICAL, Severity.HIGH -> ErrorRed
        Severity.MEDIUM -> AccentGold
        else -> AccentMint
    }
    // 新增：evidence 独立展开状态
    var showFullEvidence by remember { mutableStateOf(false) }

    Row(
        modifier = Modifier.fillMaxWidth(),
        verticalAlignment = Alignment.Top
    ) {
        Box(Modifier.size(5.dp).background(sevColor, CircleShape).offset(y = 6.dp))
        Spacer(Modifier.width(10.dp))
        Column(modifier = Modifier.weight(1f)) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(finding.description, fontSize = 12.sp, fontWeight = FontWeight.Medium,
                    modifier = Modifier.weight(1f))
                Text(finding.severity.name, fontSize = 9.sp, color = sevColor,
                    fontWeight = FontWeight.Bold)
            }
            if (finding.evidence.isNotBlank()) {
                Spacer(Modifier.height(2.dp))
                Text(
                    finding.evidence,
                    fontSize = 10.sp,
                    color = TextTertiary,
                    maxLines = if (showFullEvidence) Int.MAX_VALUE else 3,
                    overflow = if (showFullEvidence) androidx.compose.ui.text.style.TextOverflow.Visible
                               else androidx.compose.ui.text.style.TextOverflow.Ellipsis,
                    modifier = Modifier.clickable { showFullEvidence = !showFullEvidence }
                )
                // 新增：展开/收起提示
                if (finding.evidence.length > 100) {
                    Text(
                        if (showFullEvidence) "收起" else "展开全部",
                        fontSize = 9.sp,
                        color = AccentPurple,
                        modifier = Modifier.clickable { showFullEvidence = !showFullEvidence }
                    )
                }
            }
        }
    }
}

/**
 * 将报告格式化为纯文本（用于分享）。
 */
private fun formatReportAsText(report: GlobalSecureReport): String {
    val sb = StringBuilder()
    sb.appendLine("=== APEX-Root 安全报告 ===")
    sb.appendLine()
    sb.appendLine("总体风险: ${report.overallRisk.name}")
    sb.appendLine("风险评分: ${"%.1f".format(report.riskScore)}%")
    val passed = report.results.count { it.success }
    sb.appendLine("层级通过: $passed / ${report.results.size}")
    sb.appendLine()
    sb.appendLine("--- 各层详情 ---")
    report.results.forEach { result ->
        val status = if (result.success) "✅ 通过" else "❌ 异常"
        sb.appendLine("${result.serviceName} [$status] (${(result.confidence * 100).toInt()}%, ${result.durationMs}ms)")
        result.findings.forEach { finding ->
            sb.appendLine("  · [${finding.severity.name}] ${finding.description}")
            if (finding.evidence.isNotBlank()) {
                sb.appendLine("    证据: ${finding.evidence.take(200)}")
            }
        }
    }
    sb.appendLine()
    sb.appendLine("---")
    sb.appendLine("由 APEX-Root 生成")
    return sb.toString()
}
