package com.apex.root.ui.compose.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
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
    val hColor = if (isDark) TextPrimary else LightTextPrimary

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

            Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(48.dp)
                        .pristineGlass(cornerRadius = 14.dp)
                        .padding(horizontal = 16.dp),
                    contentAlignment = Alignment.CenterStart
                ) {
                    Text(
                        "过滤日志关键字...",
                        color = if (isDark) TextTertiary else Color(0xFF94A3B8),
                        fontSize = 14.sp
                    )
                }

                Spacer(Modifier.height(16.dp))

                LazyColumn(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                    items(uiState.logs) { log ->
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
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            Box(
                                modifier = Modifier
                                    .background(tagColor.copy(alpha = 0.15f), RoundedCornerShape(6.dp))
                                    .padding(horizontal = 8.dp, vertical = 4.dp)
                            ) {
                                Text(log.type.name, color = tagColor, fontSize = 11.sp, fontWeight = FontWeight.Bold)
                            }
                            Spacer(Modifier.width(12.dp))
                            Text(log.time, color = if (isDark) TextTertiary else Color(0xFF94A3B8), fontSize = 12.sp)
                            Spacer(Modifier.width(12.dp))
                            Text(log.message, color = hColor, fontSize = 13.sp)
                        }
                    }
                }
            }
        }
    }
}
