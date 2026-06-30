package com.apex.root.ui.compose.screens

import androidx.compose.foundation.background

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
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.apex.root.ui.compose.*

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun WhitelistScreen(
    onBack: () -> Unit = {},
    whitelist: List<String> = listOf(
        "com.google.android.gms",
        "com.android.systemui",
        "com.android.phone"
    ),
    onAddPackage: (String) -> Unit = {},
    onRemovePackage: (String) -> Unit = {}
) {
    var showDialog by remember { mutableStateOf(false) }
    var newPackage by remember { mutableStateOf("") }
    // 修复：原代码 remember { mutableStateOf(whitelist) } 不依赖 whitelist 参数作为 key，
    // 导致外部数据更新后 UI 仍显示旧数据。改为 remember(whitelist) 让 recomposition 时刷新。
    var items by remember(whitelist) { mutableStateOf(whitelist) }

    val scrollBehavior = TopAppBarDefaults.exitUntilCollapsedScrollBehavior()

    Scaffold(
        modifier = Modifier.nestedScroll(scrollBehavior.nestedScrollConnection),
        containerColor = Color.Transparent,
        topBar = {
            CollapsibleGlassTopBar(
                title = "白名单管理",
                collapsedFraction = scrollBehavior.state.collapsedFraction,
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.Default.ArrowBack, "返回")
                    }
                },
                actions = {
                    IconButton(onClick = { showDialog = true }) {
                        Icon(Icons.Default.Add, "添加", tint = AccentPurple)
                    }
                }
            )
        }
    ) { padding ->
        LiquidGlassContainer(
            fluidColorsDark = PageFluidColors.settings,
            fluidColorsLight = PageFluidColors.settingsLight
        ) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(padding)
                    .padding(16.dp)
            ) {
                Spacer(Modifier.height(4.dp))

                GlassCard(cornerRadius = 16.dp) {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        GlassIconBox(icon = Icons.Default.CheckCircle, accentColor = AccentMint, size = 36.dp, iconSize = 18.dp)
                        Spacer(Modifier.width(14.dp))
                        Text("信任应用白名单，排除正常系统进程误报", fontSize = 12.sp, color = TextSecondary)
                    }
                }

                Spacer(Modifier.height(16.dp))

                if (items.isEmpty()) {
                    GlassEmptyState(
                        title = "白名单为空",
                        description = "点击右上角 + 添加信任应用包名",
                        icon = {
                            Box(
                                Modifier.size(80.dp)
                                    .background(if (LocalIsDarkTheme.current) AccentMint.copy(alpha = 0.15f) else PastelMint, shape = RoundedCornerShape(20.dp)),
                                contentAlignment = Alignment.Center
                            ) {
                                Icon(Icons.Default.CheckCircle, null, Modifier.size(40.dp), tint = AccentMint)
                            }
                        }
                    )
                } else {
                    Row(
                        modifier = Modifier.fillMaxWidth().padding(horizontal = 4.dp, vertical = 8.dp),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        SectionHeader(title = "共 ${items.size} 个白名单应用")
                    }
                    Spacer(Modifier.height(4.dp))

                    // 修复：LazyColumn 加 weight(1f) 让它填充剩余空间并可滚动
                    // 加 key 参数提升 Compose 性能
                    LazyColumn(
                        verticalArrangement = Arrangement.spacedBy(8.dp),
                        modifier = Modifier.weight(1f).fillMaxWidth()
                    ) {
                        items(items, key = { it }) { pkg ->
                            GlassCard(cornerRadius = 14.dp) {
                                Row(
                                    modifier = Modifier.fillMaxWidth(),
                                    verticalAlignment = Alignment.CenterVertically
                                ) {
                                    GlassIconBox(icon = Icons.Default.CheckCircle, accentColor = AccentMint, size = 32.dp, iconSize = 16.dp)
                                    Spacer(Modifier.width(12.dp))
                                    Text(
                                        pkg,
                                        modifier = Modifier.weight(1f),
                                        fontWeight = FontWeight.Medium,
                                        fontSize = 13.sp,
                                        color = TextPrimary,
                                        maxLines = 2,
                                        overflow = androidx.compose.ui.text.style.TextOverflow.Ellipsis
                                    )
                                    IconButton(onClick = {
                                        items = items - pkg
                                        onRemovePackage(pkg)
                                    }) {
                                        Icon(Icons.Default.Delete, "删除", tint = ErrorRed.copy(alpha = 0.7f), modifier = Modifier.size(20.dp))
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (showDialog) {
        AlertDialog(
            onDismissRequest = { showDialog = false },
            containerColor = if (LocalIsDarkTheme.current) DeepSurface else Color.White,
            title = { Text("添加白名单", fontWeight = FontWeight.Bold) },
            text = {
                OutlinedTextField(
                    value = newPackage,
                    onValueChange = { newPackage = it },
                    label = { Text("包名") },
                    singleLine = true,
                    modifier = Modifier.fillMaxWidth(),
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentPurple,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f)
                    ),
                    shape = RoundedCornerShape(10.dp)
                )
            },
            confirmButton = {
                Button(
                    onClick = {
                        if (newPackage.isNotBlank() && newPackage !in items) {
                            items = items + newPackage
                            onAddPackage(newPackage)
                        }
                        newPackage = ""
                        showDialog = false
                    },
                    colors = ButtonDefaults.buttonColors(containerColor = AccentPurple),
                    shape = RoundedCornerShape(10.dp)
                ) { Text("添加") }
            },
            dismissButton = {
                TextButton(onClick = { showDialog = false }) { Text("取消") }
            }
        )
    }
}
