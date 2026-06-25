package com.apex.root.ui.compose.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.apex.root.ui.compose.*

@Composable
fun GlassPermissionGuideScreen(onFinished: () -> Unit) {
    var currentStep by remember { mutableIntStateOf(1) }
    val totalSteps = 2
    val isDark = LocalIsDarkTheme.current
    val bg = if (isDark) DeepBackground else Color(0xFFF1F5F9)
    val hColor = if (isDark) TextPrimary else Color(0xFF0F172A)

    Box(modifier = Modifier.fillMaxSize().background(bg), contentAlignment = Alignment.Center) {
        Column(
            modifier = Modifier
                .padding(24.dp)
                .fillMaxWidth()
                .pristineGlass(cornerRadius = 32.dp)
                .padding(32.dp)
        ) {
            Text("STEP $currentStep / $totalSteps", color = Color(0xFF0D9488), fontWeight = FontWeight.Bold, fontSize = 12.sp)
            Spacer(Modifier.height(16.dp))

            if (currentStep == 1) {
                Text("授予无障碍服务权限", fontSize = 22.sp, fontWeight = FontWeight.Bold, color = hColor)
                Spacer(Modifier.height(12.dp))
                Text("软件需要监听无障碍事件，以自动化阻断后台高耗能恶意轮询动作。",
                    color = if (isDark) TextSecondary else Color(0xFF475569), fontSize = 14.sp)
            } else {
                Text("获取 ROOT 权限高级诊断", fontSize = 22.sp, fontWeight = FontWeight.Bold, color = hColor)
                Spacer(Modifier.height(12.dp))
                Text("深度模式下需要系统超级管理员权级，以深度清空并重置电池校准状态。",
                    color = if (isDark) TextSecondary else Color(0xFF475569), fontSize = 14.sp)
            }

            Spacer(Modifier.height(32.dp))

            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                repeat(totalSteps) { idx ->
                    val active = idx + 1 == currentStep
                    Box(
                        modifier = Modifier
                            .width(if (active) 24.dp else 8.dp)
                            .height(8.dp)
                            .background(
                                if (active) Color(0xFF0D9488) else if (isDark) TextTertiary else Color(0xFFCBD5E1),
                                CircleShape
                            )
                    )
                }
            }

            Spacer(Modifier.height(40.dp))

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    "暂不授权",
                    modifier = Modifier.clickable {
                        if (currentStep < totalSteps) currentStep++ else onFinished()
                    },
                    color = if (isDark) TextTertiary else Color(0xFF94A3B8)
                )
                Button(
                    onClick = {
                        if (currentStep < totalSteps) currentStep++ else onFinished()
                    },
                    colors = ButtonDefaults.buttonColors(containerColor = if (isDark) AccentPurple else Color(0xFF0F172A))
                ) {
                    Text("去授权", color = Color.White)
                }
            }
        }
    }
}
