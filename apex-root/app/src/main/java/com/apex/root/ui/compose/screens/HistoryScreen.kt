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
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.apex.root.ui.compose.*
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

data class HistoryEntry(
    val timestamp: Long,
    val riskScore: Int,
    val rootDetected: Boolean,
    val level: String,
    val layerCount: Int
)

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun HistoryScreen(
    entries: List<HistoryEntry> = remember {
        listOf(
            HistoryEntry(System.currentTimeMillis() - 3600000, 72, true, "深度", 8),
            HistoryEntry(System.currentTimeMillis() - 86400000, 15, false, "标准", 4),
            HistoryEntry(System.currentTimeMillis() - 172800000, 88, true, "深度", 12)
        )
    },
    onBack: () -> Unit = {},
    onClearHistory: () -> Unit = {}
) {
    val scrollBehavior = TopAppBarDefaults.exitUntilCollapsedScrollBehavior()

    Scaffold(
        modifier = Modifier.nestedScroll(scrollBehavior.nestedScrollConnection),
        containerColor = Color.Transparent,
        topBar = {
            CollapsibleGlassTopBar(
                title = "检测历史",
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
            fluidColorsDark = PageFluidColors.dashboard,
            fluidColorsLight = PageFluidColors.dashboardLight
        ) {
            if (entries.isEmpty()) {
                Box(
                    modifier = Modifier
                        .fillMaxSize()
                        .padding(padding)
                        .padding(24.dp),
                    contentAlignment = Alignment.Center
                ) {
                    GlassEmptyState(
                        title = "暂无检测记录",
                        description = "运行检测后，结果将显示在这里",
                        icon = {
                            Box(
                                Modifier.size(80.dp)
                                    .background(if (LocalIsDarkTheme.current) AccentBlue.copy(alpha = 0.15f) else PastelBlue, CircleShape),
                                contentAlignment = Alignment.Center
                            ) {
                                Icon(Icons.Default.History, null, Modifier.size(40.dp), tint = AccentBlue)
                            }
                        }
                    )
                }
                return@LiquidGlassContainer
            }

            LazyColumn(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(padding)
                    .padding(horizontal = 16.dp),
                verticalArrangement = Arrangement.spacedBy(10.dp),
                contentPadding = PaddingValues(vertical = 16.dp)
            ) {
                item {
                    Row(
                        modifier = Modifier.fillMaxWidth().padding(horizontal = 4.dp),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        SectionHeader(title = "共 ${entries.size} 条记录")
                        IconButton(onClick = onClearHistory) {
                            Icon(Icons.Default.DeleteSweep, "清空", tint = TextTertiary, modifier = Modifier.size(20.dp))
                        }
                    }
                    Spacer(Modifier.height(8.dp))
                }

                items(entries) { entry ->
                    HistoryCard(entry)
                }

                item { Spacer(Modifier.height(16.dp)) }
            }
        }
    }
}

@Composable
private fun HistoryCard(entry: HistoryEntry) {
    val riskColor = when {
        entry.riskScore >= 80 -> ErrorRed
        entry.riskScore >= 50 -> AccentGold
        entry.riskScore >= 20 -> AccentMint
        else -> Color(0xFF00E676)
    }
    val dateFormat = SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault())

    GlassCard(cornerRadius = 16.dp, accentLine = riskColor) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Column(modifier = Modifier.weight(1f)) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text(
                        text = dateFormat.format(Date(entry.timestamp)),
                        fontSize = 11.sp,
                        color = TextTertiary
                    )
                    Spacer(Modifier.width(10.dp))
                    Box(
                        Modifier
                            .background(riskColor.copy(alpha = 0.15f), RoundedCornerShape(4.dp))
                            .padding(horizontal = 6.dp, vertical = 2.dp)
                    ) {
                        Text(entry.level, fontSize = 9.sp, color = riskColor, fontWeight = FontWeight.SemiBold)
                    }
                }
                Spacer(Modifier.height(6.dp))
                Row(verticalAlignment = Alignment.CenterVertically) {
                    if (entry.rootDetected) {
                        Icon(Icons.Default.Warning, null, tint = ErrorRed, modifier = Modifier.size(16.dp))
                        Spacer(Modifier.width(6.dp))
                        Text("检测到 Root", fontSize = 13.sp, fontWeight = FontWeight.Medium, color = ErrorRed)
                    } else {
                        Icon(Icons.Default.CheckCircle, null, tint = AccentMint, modifier = Modifier.size(16.dp))
                        Spacer(Modifier.width(6.dp))
                        Text("环境安全", fontSize = 13.sp, fontWeight = FontWeight.Medium, color = AccentMint)
                    }
                    Spacer(Modifier.width(16.dp))
                    Text("${entry.layerCount} 层", fontSize = 11.sp, color = TextTertiary)
                }
            }
            Column(horizontalAlignment = Alignment.End) {
                Text("${entry.riskScore}", fontSize = 22.sp, fontWeight = FontWeight.Bold, color = riskColor)
                Text("风险分", fontSize = 9.sp, color = TextTertiary)
            }
        }
    }
}
