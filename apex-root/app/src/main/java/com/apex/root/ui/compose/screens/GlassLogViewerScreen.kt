package com.apex.root.ui.compose.screens

import androidx.compose.animation.core.*
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material.icons.filled.Clear
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.Search
import androidx.compose.material.icons.filled.Terminal
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.apex.root.ui.compose.*
import com.apex.root.viewmodel.trusted.ApexViewModel
import com.apex.root.viewmodel.trusted.LogType
import kotlinx.coroutines.launch

/**
 * 终端式日志查看器
 * - 黑色背景 + monospace 字体
 * - 颜色区分 INFO/WARN/ERROR
 * - 自动滚动到最新
 * - 搜索过滤 + 级别过滤
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun GlassLogViewerScreen(viewModel: ApexViewModel, onBack: () -> Unit) {
    val uiState by viewModel.uiState.collectAsState()
    val isDark = LocalIsDarkTheme.current

    var searchQuery by remember { mutableStateOf("") }
    var selectedLevel by remember { mutableStateOf<LogType?>(null) }

    val filteredLogs = remember(uiState.logs, searchQuery, selectedLevel) {
        uiState.logs.filter { log ->
            val levelMatch = selectedLevel == null || log.type == selectedLevel
            val searchMatch = searchQuery.isBlank() ||
                    log.message.contains(searchQuery, ignoreCase = true)
            levelMatch && searchMatch
        }
    }

    // 自动滚动到底部
    val listState = rememberLazyListState()
    val scope = rememberCoroutineScope()
    LaunchedEffect(filteredLogs.size) {
        if (filteredLogs.isNotEmpty()) {
            scope.launch {
                listState.animateScrollToItem(filteredLogs.size - 1)
            }
        }
    }

    Column(modifier = Modifier.fillMaxSize().statusBarsPadding()) {
        // 顶栏
        TopAppBar(
            title = {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Icon(Icons.Default.Terminal, contentDescription = null, tint = AccentPurple, modifier = Modifier.size(20.dp))
                    Spacer(Modifier.width(8.dp))
                    Text("终端日志", fontWeight = FontWeight.Bold, color = TextPrimary)
                }
            },
            navigationIcon = {
                IconButton(onClick = onBack) {
                    Icon(Icons.Default.ArrowBack, "返回", tint = TextPrimary)
                }
            },
            actions = {
                if (uiState.logs.isNotEmpty()) {
                    IconButton(onClick = { viewModel.clearLogs() }) {
                        Icon(Icons.Default.Delete, "清空", tint = ErrorRed.copy(alpha = 0.7f), modifier = Modifier.size(20.dp))
                    }
                }
            },
            colors = TopAppBarDefaults.topAppBarColors(containerColor = Color.Transparent)
        )

        // 搜索 + 过滤栏
        Column(modifier = Modifier.padding(horizontal = 12.dp, vertical = 4.dp)) {
            OutlinedTextField(
                value = searchQuery,
                onValueChange = { searchQuery = it },
                modifier = Modifier.fillMaxWidth(),
                placeholder = { Text("过滤日志...", color = TextTertiary, fontSize = 13.sp, fontFamily = FontFamily.Monospace) },
                leadingIcon = { Icon(Icons.Default.Search, null, tint = TextTertiary, modifier = Modifier.size(16.dp)) },
                trailingIcon = {
                    if (searchQuery.isNotEmpty()) {
                        IconButton(onClick = { searchQuery = "" }) {
                            Icon(Icons.Default.Clear, "清空", tint = TextTertiary, modifier = Modifier.size(14.dp))
                        }
                    }
                },
                singleLine = true,
                shape = RoundedCornerShape(10.dp),
                colors = OutlinedTextFieldDefaults.colors(
                    focusedBorderColor = AccentPurple,
                    unfocusedBorderColor = Color.White.copy(alpha = 0.08f),
                    cursorColor = AccentPurple,
                    focusedTextColor = TextPrimary,
                    unfocusedTextColor = TextPrimary
                )
            )
            Spacer(Modifier.height(6.dp))
            Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                LogFilterChip("ALL", selectedLevel == null, AccentPurple) { selectedLevel = null }
                LogFilterChip("INFO", selectedLevel == LogType.INFO, Color(0xFF64748B)) {
                    selectedLevel = if (selectedLevel == LogType.INFO) null else LogType.INFO
                }
                LogFilterChip("WARN", selectedLevel == LogType.WARN, Color(0xFFF59E0B)) {
                    selectedLevel = if (selectedLevel == LogType.WARN) null else LogType.WARN
                }
                LogFilterChip("ERROR", selectedLevel == LogType.ERROR, Color(0xFFEF4444)) {
                    selectedLevel = if (selectedLevel == LogType.ERROR) null else LogType.ERROR
                }
            }
        }

        // 终端输出区
        Box(
            modifier = Modifier
                .weight(1f)
                .fillMaxWidth()
                .padding(8.dp)
                .clip(RoundedCornerShape(12.dp))
                .background(TermuxBg)
        ) {
            if (filteredLogs.isEmpty()) {
                // 空状态
                Column(
                    modifier = Modifier.fillMaxSize(),
                    verticalArrangement = Arrangement.Center,
                    horizontalAlignment = Alignment.CenterHorizontally
                ) {
                    // 闪烁光标
                    val infiniteTransition = rememberInfiniteTransition(label = "cursor")
                    val alpha by infiniteTransition.animateFloat(
                        initialValue = 0f, targetValue = 1f,
                        animationSpec = infiniteRepeatable(animation = tween(500), repeatMode = RepeatMode.Reverse),
                        label = "blink"
                    )
                    Text(
                        if (uiState.logs.isEmpty()) "$ 等待扫描输出..." else "$ 无匹配结果",
                        color = TermuxGreen.copy(alpha = if (uiState.logs.isEmpty()) alpha else 0.5f),
                        fontSize = 13.sp,
                        fontFamily = FontFamily.Monospace
                    )
                }
            } else {
                // 终端日志列表
                LazyColumn(
                    state = listState,
                    modifier = Modifier.fillMaxSize().padding(8.dp),
                    verticalArrangement = Arrangement.spacedBy(2.dp)
                ) {
                    items(filteredLogs, key = { it.time + it.message.hashCode() }) { log ->
                        val tagColor = when (log.type) {
                            LogType.ERROR -> Color(0xFFEF4444)
                            LogType.WARN -> Color(0xFFF59E0B)
                            LogType.INFO -> TermuxGreen
                        }
                        val tagText = when (log.type) {
                            LogType.ERROR -> "E"
                            LogType.WARN -> "W"
                            LogType.INFO -> "I"
                        }
                        Row(modifier = Modifier.fillMaxWidth()) {
                            // 时间戳
                            Text(
                                log.time,
                                color = Color(0xFF64748B),
                                fontSize = 11.sp,
                                fontFamily = FontFamily.Monospace
                            )
                            Spacer(Modifier.width(6.dp))
                            // 级别标记
                            Text(
                                "[$tagText]",
                                color = tagColor,
                                fontSize = 11.sp,
                                fontFamily = FontFamily.Monospace,
                                fontWeight = FontWeight.Bold
                            )
                            Spacer(Modifier.width(6.dp))
                            // 消息内容（不限制行数，完整显示）
                            Text(
                                log.message,
                                color = when (log.type) {
                                    LogType.ERROR -> Color(0xFFEF4444)
                                    LogType.WARN -> Color(0xFFF59E0B)
                                    LogType.INFO -> Color(0xFFE2E8F0)
                                },
                                fontSize = 12.sp,
                                fontFamily = FontFamily.Monospace,
                                lineHeight = 16.sp
                            )
                        }
                    }
                    // 底部闪烁光标
                    item {
                        val infiniteTransition = rememberInfiniteTransition(label = "cursor_end")
                        val alpha by infiniteTransition.animateFloat(
                            initialValue = 0f, targetValue = 1f,
                            animationSpec = infiniteRepeatable(animation = tween(500), repeatMode = RepeatMode.Reverse),
                            label = "blink_end"
                        )
                        Text(
                            "$ _",
                            color = TermuxGreen.copy(alpha = alpha),
                            fontSize = 13.sp,
                            fontFamily = FontFamily.Monospace
                        )
                    }
                }
            }
        }

        // 底部状态栏
        Row(
            modifier = Modifier.fillMaxWidth().padding(horizontal = 12.dp, vertical = 4.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                "${filteredLogs.size}/${uiState.logs.size} 条",
                color = TextTertiary,
                fontSize = 10.sp,
                fontFamily = FontFamily.Monospace
            )
            Row(verticalAlignment = Alignment.CenterVertically) {
                Box(Modifier.size(6.dp).clip(RoundedCornerShape(3.dp)).background(if (uiState.isScanning) AccentMint else TextTertiary))
                Spacer(Modifier.width(4.dp))
                Text(
                    if (uiState.isScanning) "RUNNING" else "IDLE",
                    color = if (uiState.isScanning) AccentMint else TextTertiary,
                    fontSize = 10.sp,
                    fontFamily = FontFamily.Monospace,
                    fontWeight = FontWeight.Bold
                )
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun LogFilterChip(label: String, selected: Boolean, color: Color, onClick: () -> Unit) {
    FilterChip(
        selected = selected,
        onClick = onClick,
        label = { Text(label, fontSize = 10.sp, fontFamily = FontFamily.Monospace) },
        shape = RoundedCornerShape(8.dp),
        colors = FilterChipDefaults.filterChipColors(
            selectedContainerColor = color.copy(alpha = 0.2f),
            selectedLabelColor = color,
            containerColor = Color.Transparent,
            labelColor = TextTertiary
        )
    )
}
