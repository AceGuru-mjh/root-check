package com.apex.root.ui.compose.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
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
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.apex.root.ui.compose.*
import com.apex.root.viewmodel.trusted.ScanViewModel
import com.apex.root.viewmodel.trusted.UiState

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun AlertScreen(
    viewModel: ScanViewModel,
    onBack: () -> Unit
) {
    val uiState by viewModel.uiState.collectAsState()
    val alert = (uiState as? UiState.Alert)?.alert

    Scaffold(
        containerColor = Color.Transparent,
        topBar = {
            TopAppBar(
                title = { Text("安全警报", fontWeight = FontWeight.Bold, letterSpacing = 0.5.sp) },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.Default.ArrowBack, "返回")
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(containerColor = Color.Transparent)
            )
        }
    ) { padding ->
        LiquidGlassContainer(fluidColorsDark = PageFluidColors.alert, fluidColorsLight = PageFluidColors.alertLight) {
        Box(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
                .padding(24.dp),
            contentAlignment = Alignment.Center
        ) {
            if (alert == null) {
                GlassEmptyState(
                    title = "无活跃警报",
                    description = "系统运行正常，未检测到任何异常指标"
                )
                return@Box
            }

            GlassCard(cornerRadius = 24.dp, accentLine = ErrorRed) {
                Column(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalAlignment = Alignment.CenterHorizontally
                ) {
                    Box(
                        modifier = Modifier
                            .size(72.dp)
                            .clip(CircleShape)
                            .liquidGlassIcon(36.dp, ErrorRed.copy(alpha = 0.15f)),
                        contentAlignment = Alignment.Center
                    ) {
                        Icon(Icons.Default.Shield, null, Modifier.size(36.dp), tint = ErrorRed)
                    }
                    Spacer(Modifier.height(18.dp))
                    Text("严重警报", fontSize = 22.sp, fontWeight = FontWeight.Bold,
                        color = ErrorRed, letterSpacing = 0.5.sp)
                    Spacer(Modifier.height(10.dp))
                    Text(alert.description, style = MaterialTheme.typography.bodyMedium,
                        color = TextPrimary)
                    Spacer(Modifier.height(24.dp))
                    Box(Modifier.fillMaxWidth().height(0.5.dp)
                        .background(Color.White.copy(alpha = 0.05f)))
                    Spacer(Modifier.height(18.dp))
                    DetailRow("类型", alert.type.name)
                    DetailRow("来源", alert.sourceReplica)
                    Spacer(Modifier.height(28.dp))
                    Button(
                        onClick = onBack,
                        shape = RoundedCornerShape(12.dp),
                        colors = ButtonDefaults.buttonColors(containerColor = ErrorRed),
                        contentPadding = PaddingValues(horizontal = 28.dp, vertical = 12.dp)
                    ) {
                        Icon(Icons.Default.Check, null, Modifier.size(18.dp))
                        Spacer(Modifier.width(8.dp))
                        Text("确认", fontWeight = FontWeight.SemiBold, letterSpacing = 0.3.sp)
                    }
                }
            }
        }
    }
    }
}

@Composable
private fun DetailRow(label: String, value: String) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 4.dp),
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(label, color = TextTertiary, fontSize = 12.sp)
        Text(value, color = TextPrimary, fontWeight = FontWeight.Medium, fontSize = 12.sp)
    }
}
