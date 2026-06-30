package com.apex.root.ui.compose.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material.icons.filled.Clear
import androidx.compose.material.icons.filled.Search
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.apex.root.ui.compose.*
import com.apex.root.viewmodel.trusted.ApexViewModel
import com.apex.root.viewmodel.trusted.LogType

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun GlassLogViewerScreen(viewModel: ApexViewModel, onBack: () -> Unit) {
    val uiState by viewModel.uiState.collectAsState()
    val isDark = LocalIsDarkTheme.current
    val hColor = if (isDark) TextPrimary else TextPrimary

    // 修复：搜索框改为真正的 OutlinedTextField + 实现过滤逻辑
    var searchQuery by remember { mutableStateOf("") }
    var selectedLevel by remember { mutableStateOf<LogType?>(null) }

    // 过滤后的日志
    val filteredLogs = remember(uiState.logs, searchQuery, selectedLevel) {
        uiState.logs.filter { log ->
            val levelMatch = selectedLevel == null || log.type == selectedLevel
            val searchMatch = searchQuery.isBlank() ||
                    log.message.contains(searchQuery, ignoreCase = true) ||
                    log.type.name.contains(searchQuery, ignoreCase = true)
            levelMatch && searchMatch
        }
    }

    LiquidGlassContainer(
        fluidColorsDark = PageFluidColors.settings,
        fluidColorsLight = PageFluidColors.settingsLight
    ) {
        Column(modifier = Modifier.fillMaxSize().statusBarsPadding()) {
            TopAppBar(
                title = { Text("诊断日志", fontWeight = FontWeight.Bold, color = hColor) },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.Default.ArrowBack, "返回", tint = hColor)
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(containerColor = Color.Transparent)
            )

            Column(modifier = Modifier.fillMaxSize().padding(horizontal = 16.dp)) {
                // 搜索框：真正的 OutlinedTextField
                OutlinedTextField(
                    value = searchQuery,
                    onValueChange = { searchQuery = it },
                    modifier = Modifier.fillMaxWidth(),
                    placeholder = {
                        Text(
                            "过滤日志关键字...",
                            color = if (isDark) TextTertiary else Color(0xFF94A3B8),
                            fontSize = 14.sp
                        )
                    },
                    leadingIcon = {
                        Icon(
                            Icons.Default.Search,
                            contentDescription = null,
                            tint = if (isDark) TextTertiary else Color(0xFF94A3B8),
                            modifier = Modifier.size(18.dp)
                        )
                    },
                    trailingIcon = {
                        if (searchQuery.isNotEmpty()) {
                            IconButton(onClick = { searchQuery = "" }) {
                                Icon(
                                    Icons.Default.Clear,
                                    contentDescription = "清空",
                                    tint = if (isDark) TextTertiary else Color(0xFF94A3B8),
                                    modifier = Modifier.size(16.dp)
                                )
                            }
                        }
                    },
                    singleLine = true,
                    shape = RoundedCornerShape(14.dp),
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentPurple,
                        unfocusedBorderColor = if (isDark) Color.White.copy(alpha = 0.1f) else Color.Black.copy(alpha = 0.1f),
                        cursorColor = AccentPurple,
                        focusedTextColor = hColor,
                        unfocusedTextColor = hColor
                    )
                )

                Spacer(Modifier.height(12.dp))

                // 日志级别过滤芯片
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    FilterChip(
                        selected = selectedLevel == null,
                        onClick = { selectedLevel = null },
                        label = { Text("全部", fontSize = 11.sp) },
                        shape = RoundedCornerShape(12.dp),
                        colors = FilterChipDefaults.filterChipColors(
                            selectedContainerColor = AccentPurple.copy(alpha = 0.2f),
                            selectedLabelColor = AccentPurple
                        )
                    )
                    FilterChip(
                        selected = selectedLevel == LogType.INFO,
                        onClick = { selectedLevel = if (selectedLevel == LogType.INFO) null else LogType.INFO },
                        label = { Text("INFO", fontSize = 11.sp) },
                        shape = RoundedCornerShape(12.dp),
                        colors = FilterChipDefaults.filterChipColors(
                            selectedContainerColor = Color(0xFF64748B).copy(alpha = 0.2f),
                            selectedLabelColor = Color(0xFF64748B)
                        )
                    )
                    FilterChip(
                        selected = selectedLevel == LogType.WARN,
                        onClick = { selectedLevel = if (selectedLevel == LogType.WARN) null else LogType.WARN },
                        label = { Text("WARN", fontSize = 11.sp) },
                        shape = RoundedCornerShape(12.dp),
                        colors = FilterChipDefaults.filterChipColors(
                            selectedContainerColor = Color(0xFFF59E0B).copy(alpha = 0.2f),
                            selectedLabelColor = Color(0xFFF59E0B)
                        )
                    )
                    FilterChip(
                        selected = selectedLevel == LogType.ERROR,
                        onClick = { selectedLevel = if (selectedLevel == LogType.ERROR) null else LogType.ERROR },
                        label = { Text("ERROR", fontSize = 11.sp) },
                        shape = RoundedCornerShape(12.dp),
                        colors = FilterChipDefaults.filterChipColors(
                            selectedContainerColor = Color(0xFFEF4444).copy(alpha = 0.2f),
                            selectedLabelColor = Color(0xFFEF4444)
                        )
                    )
                }

                Spacer(Modifier.height(8.dp))

                // 结果计数
                Text(
                    "共 ${filteredLogs.size} 条日志" +
                            if (filteredLogs.size != uiState.logs.size) " (已从 ${uiState.logs.size} 条过滤)" else "",
                    fontSize = 11.sp,
                    color = if (isDark) TextTertiary else Color(0xFF94A3B8),
                    modifier = Modifier.padding(vertical = 4.dp)
                )

                Spacer(Modifier.height(8.dp))

                if (filteredLogs.isEmpty()) {
                    // 空状态
                    Box(
                        modifier = Modifier.fillMaxSize().weight(1f),
                        contentAlignment = Alignment.Center
                    ) {
                        Column(horizontalAlignment = Alignment.CenterHorizontally) {
                            Icon(
                                Icons.Default.Search,
                                contentDescription = null,
                                tint = if (isDark) TextTertiary else Color(0xFF94A3B8),
                                modifier = Modifier.size(48.dp)
                            )
                            Spacer(Modifier.height(12.dp))
                            Text(
                                if (uiState.logs.isEmpty()) "暂无日志" else "无匹配结果",
                                fontSize = 14.sp,
                                color = if (isDark) TextTertiary else Color(0xFF94A3B8)
                            )
                            if (searchQuery.isNotEmpty()) {
                                Spacer(Modifier.height(4.dp))
                                Text(
                                    "尝试其他关键字",
                                    fontSize = 12.sp,
                                    color = if (isDark) TextTertiary else Color(0xFF94A3B8)
                                )
                            }
                        }
                    }
                } else {
                    // 修复：LazyColumn 加 weight(1f) + items 加 key
                    LazyColumn(
                        verticalArrangement = Arrangement.spacedBy(8.dp),
                        modifier = Modifier.weight(1f).fillMaxWidth()
                    ) {
                        items(filteredLogs, key = { it.time + it.message.hashCode() }) { log ->
                            val tagColor = when (log.type) {
                                LogType.ERROR -> Color(0xFFEF4444)
                                LogType.WARN -> Color(0xFFF59E0B)
                                LogType.INFO -> if (isDark) TextTertiary else Color(0xFF64748B)
                            }
                            Row(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .pristineGlass(cornerRadius = 12.dp)
                                    .padding(12.dp),
                                verticalAlignment = Alignment.Top  // 修复：多行日志对齐到顶部
                            ) {
                                Box(
                                    modifier = Modifier
                                        .background(tagColor.copy(alpha = 0.15f), RoundedCornerShape(6.dp))
                                        .padding(horizontal = 8.dp, vertical = 4.dp)
                                ) {
                                    Text(log.type.name, color = tagColor, fontSize = 11.sp, fontWeight = FontWeight.Bold)
                                }
                                Spacer(Modifier.width(12.dp))
                                Text(
                                    log.time,
                                    color = if (isDark) TextTertiary else Color(0xFF94A3B8),
                                    fontSize = 12.sp
                                )
                                Spacer(Modifier.width(12.dp))
                                // 修复：长日志限制 maxLines + overflow
                                Text(
                                    log.message,
                                    color = hColor,
                                    fontSize = 13.sp,
                                    maxLines = 4,
                                    overflow = TextOverflow.Ellipsis,
                                    modifier = Modifier.weight(1f)
                                )
                            }
                        }
                    }
                }
            }
        }
    }
}
