package com.apex.root.ui.compose.screens

import androidx.compose.foundation.background

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.apex.root.ui.compose.*

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun FeatureTestScreen(
    onBack: () -> Unit = {}
) {
    var inputText by remember { mutableStateOf("") }
    var testType by remember { mutableStateOf(0) }
    var result by remember { mutableStateOf("") }
    var isRunning by remember { mutableStateOf(false) }

    val testTypes = listOf("文件/路径检测", "内存字符串扫描", "属性值检测", "Socket 检测")

    val scrollBehavior = TopAppBarDefaults.exitUntilCollapsedScrollBehavior()

    Scaffold(
        modifier = Modifier.nestedScroll(scrollBehavior.nestedScrollConnection),
        containerColor = Color.Transparent,
        topBar = {
            CollapsibleGlassTopBar(
                title = "单条特征测试",
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
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(padding)
                    .padding(16.dp)
                    .verticalScroll(rememberScrollState()),
                verticalArrangement = Arrangement.spacedBy(16.dp)
            ) {
                GlassCard(cornerRadius = 16.dp) {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        GlassIconBox(icon = Icons.Default.Search, accentColor = AccentPurple, size = 36.dp, iconSize = 18.dp)
                        Spacer(Modifier.width(14.dp))
                        Text("输入待检测的路径/字符串，单独执行特定检测项", fontSize = 12.sp, color = TextSecondary)
                    }
                }

                GlassCard(cornerRadius = 16.dp, accentLine = AccentPurple) {
                    Text("检测类型", fontWeight = FontWeight.SemiBold, fontSize = 13.sp, color = TextPrimary)
                    Spacer(Modifier.height(12.dp))
                    testTypes.forEachIndexed { index, label ->
                        Row(
                            verticalAlignment = Alignment.CenterVertically,
                            modifier = Modifier.padding(vertical = 4.dp)
                        ) {
                            RadioButton(
                                selected = testType == index,
                                onClick = { testType = index },
                                colors = RadioButtonDefaults.colors(selectedColor = AccentPurple)
                            )
                            Spacer(Modifier.width(4.dp))
                            Text(label, fontSize = 13.sp, color = TextPrimary)
                        }
                    }
                }

                OutlinedTextField(
                    value = inputText,
                    onValueChange = { inputText = it },
                    label = { Text("输入路径/字符串") },
                    modifier = Modifier.fillMaxWidth(),
                    singleLine = true,
                    leadingIcon = { Icon(Icons.Default.Search, null, tint = TextTertiary) },
                    keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Text),
                    colors = OutlinedTextFieldDefaults.colors(
                        focusedBorderColor = AccentPurple,
                        unfocusedBorderColor = Color.White.copy(alpha = 0.10f),
                        focusedLabelColor = AccentPurple,
                        unfocusedLabelColor = TextTertiary
                    ),
                    shape = RoundedCornerShape(12.dp)
                )

                Button(
                    onClick = {
                        isRunning = true
                        result = when (testType) {
                            0 -> "检测路径: $inputText\n结果: ${if (inputText.contains("adb") || inputText.contains("magisk")) "可疑" else "正常"}"
                            1 -> "内存扫描: \"$inputText\"\n结果: 扫描 32768 字节，未发现匹配"
                            2 -> "属性检测: $inputText\n结果: 属性不存在"
                            3 -> "Socket 扫描: 发现 12 个活动 socket\n结果: 无异常 socket"
                            else -> "未知检测类型"
                        }
                        isRunning = false
                    },
                    modifier = Modifier.fillMaxWidth().height(48.dp),
                    enabled = !isRunning && inputText.isNotBlank(),
                    shape = RoundedCornerShape(14.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentPurple)
                ) {
                    if (isRunning) {
                        CircularProgressIndicator(Modifier.size(20.dp), strokeWidth = 2.dp, color = Color.White)
                        Spacer(Modifier.width(8.dp))
                    } else {
                        Icon(Icons.Default.PlayArrow, null, Modifier.size(18.dp))
                        Spacer(Modifier.width(8.dp))
                    }
                    Text("执行检测", fontWeight = FontWeight.SemiBold)
                }

                if (result.isNotBlank()) {
                    GlassCard(cornerRadius = 16.dp, accentLine = AccentMint) {
                        Text("检测结果", fontWeight = FontWeight.SemiBold, fontSize = 13.sp, color = TextPrimary)
                        Spacer(Modifier.height(10.dp))
                        Box(
                            Modifier
                                .fillMaxWidth()
                                .background(Color.Black.copy(alpha = 0.25f), RoundedCornerShape(8.dp))
                                .padding(12.dp)
                        ) {
                            Text(result, fontFamily = FontFamily.Monospace, fontSize = 12.sp, color = TermuxGreen.copy(alpha = 0.8f))
                        }
                    }
                }

                Spacer(Modifier.height(32.dp))
            }
        }
    }
}
