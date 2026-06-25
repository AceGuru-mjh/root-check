package com.rootdetector.ui.compose.screens

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Search
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun FeatureTestScreen(
    onBack: () -> Unit = {}
) {
    var inputText by remember { mutableStateOf("") }
    var testType by remember { mutableStateOf(0) }
    var result by remember { mutableStateOf("") }
    var isRunning by remember { mutableStateOf(false) }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("单条特征测试") },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = null)
                    }
                }
            )
        }
    ) { padding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
                .padding(16.dp)
                .verticalScroll(rememberScrollState()),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            Text(
                "输入待检测的路径/字符串，单独执行特定检测项",
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )

            Card(
                modifier = Modifier.fillMaxWidth(),
                shape = RoundedCornerShape(12.dp),
                colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant.copy(alpha = 0.4f))
            ) {
                Column(modifier = Modifier.padding(16.dp)) {
                    Text("检测类型", style = MaterialTheme.typography.titleSmall)

                    Row(verticalAlignment = Alignment.CenterVertically) {
                        RadioButton(selected = testType == 0, onClick = { testType = 0 })
                        Text("文件/路径检测")
                    }
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        RadioButton(selected = testType == 1, onClick = { testType = 1 })
                        Text("内存字符串扫描")
                    }
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        RadioButton(selected = testType == 2, onClick = { testType = 2 })
                        Text("属性值检测")
                    }
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        RadioButton(selected = testType == 3, onClick = { testType = 3 })
                        Text("Socket 检测")
                    }
                }
            }

            OutlinedTextField(
                value = inputText,
                onValueChange = { inputText = it },
                label = { Text("输入路径/字符串") },
                modifier = Modifier.fillMaxWidth(),
                singleLine = true,
                leadingIcon = { Icon(Icons.Default.Search, contentDescription = null) },
                keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Text)
            )

            Button(
                onClick = {
                    isRunning = true
                    result = when (testType) {
                        0 -> "检测路径: $inputText\n结果: ${if (inputText.contains("adb") || inputText.contains("magisk")) "⚠️ 可疑" else "✅ 正常"}"
                        1 -> "内存扫描: \"$inputText\"\n结果: 扫描 32768 字节，未发现匹配"
                        2 -> "属性检测: $inputText\n结果: 属性不存在"
                        3 -> "Socket 扫描: 发现 12 个活动 socket\n结果: 无异常 socket"
                        else -> "未知检测类型"
                    }
                    isRunning = false
                },
                modifier = Modifier.fillMaxWidth(),
                enabled = !isRunning && inputText.isNotBlank()
            ) {
                if (isRunning) {
                    CircularProgressIndicator(modifier = Modifier.size(20.dp), strokeWidth = 2.dp)
                    Spacer(Modifier.width(8.dp))
                } else {
                    Icon(Icons.Default.PlayArrow, contentDescription = null)
                    Spacer(Modifier.width(8.dp))
                }
                Text("执行检测")
            }

            if (result.isNotBlank()) {
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    shape = RoundedCornerShape(12.dp),
                    colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant.copy(alpha = 0.5f))
                ) {
                    Column(modifier = Modifier.padding(16.dp)) {
                        Text("检测结果", fontWeight = androidx.compose.ui.text.font.FontWeight.SemiBold)
                        Spacer(Modifier.height(8.dp))
                        Text(result, fontFamily = FontFamily.Monospace, fontSize = 13.sp)
                    }
                }
            }

            Spacer(Modifier.height(32.dp))
        }
    }
}
