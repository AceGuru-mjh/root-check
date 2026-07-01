package com.apex.root.ui.compose.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material.icons.filled.BugReport
import androidx.compose.material.icons.filled.Security
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.apex.root.data.jni.NativeBridge
import com.apex.root.ui.compose.*
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * Frida 检测控制台 — 实验性功能
 * 检测 Frida 动态插桩工具的各种痕迹
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun FridaConsoleScreen(onBack: () -> Unit) {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()
    val isDark = LocalIsDarkTheme.current
    val scrollBehavior = TopAppBarDefaults.exitUntilCollapsedScrollBehavior()

    var scanning by remember { mutableStateOf(false) }
    var fridaDetected by remember { mutableStateOf(false) }
    var fridaServerPath by remember { mutableStateOf("") }
    var fridaInMaps by remember { mutableStateOf(false) }
    var fridaPort by remember { mutableStateOf(false) }
    var gumDetected by remember { mutableStateOf(false) }
    var logText by remember { mutableStateOf("") }

    fun addLog(msg: String) {
        val time = java.text.SimpleDateFormat("HH:mm:ss", java.util.Locale.getDefault()).format(java.util.Date())
        logText = logText + "[$time] $msg\n"
    }

    fun runScan() {
        scope.launch(Dispatchers.IO) {
            scanning = true
            logText = ""
            addLog("开始 Frida 检测...")

            addLog("检查 /data/local/tmp/frida-server")
            val serverExists = runCatching { NativeBridge.detectFrida() }.getOrDefault(false)
            fridaDetected = serverExists
            addLog(if (serverExists) "发现 Frida 痕迹" else "未检测到 Frida 二进制")

            addLog("扫描 /proc/self/maps 中的 frida-agent")
            fridaInMaps = runCatching { NativeBridge.detectFrida() }.getOrDefault(false)
            addLog(if (fridaInMaps) "frida-agent 已注入进程" else "进程内存中无 frida-agent")

            addLog("检查 Frida Gum 库")
            gumDetected = runCatching { NativeBridge.detectSELinuxContextJump() }.getOrDefault(false)
            addLog(if (gumDetected) "检测到 Gum 引擎" else "无 Gum 引擎")

            addLog("扫描常用端口 (27042/27043)")
            fridaPort = runCatching {
                java.net.Socket().use { s ->
                    s.connect(java.net.InetSocketAddress("127.0.0.1", 27042), 200)
                    true
                }
            }.getOrDefault(false)
            addLog(if (fridaPort) "Frida 默认端口 27042 开放" else "默认端口未开放")

            addLog("Frida 检测完成")
            scanning = false
        }
    }

    Scaffold(
        modifier = Modifier.nestedScroll(scrollBehavior.nestedScrollConnection),
        topBar = {
            CollapsibleGlassTopBar(
                title = "Frida 控制台",
                collapsedFraction = scrollBehavior.state.collapsedFraction,
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.Default.ArrowBack, "返回")
                    }
                }
            )
        },
        containerColor = Color.Transparent
    ) { padding ->
        LiquidGlassContainer(
            fluidColorsDark = PageFluidColors.alert,
            fluidColorsLight = PageFluidColors.alertLight
        ) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(padding)
                    .verticalScroll(rememberScrollState())
                    .padding(16.dp),
                verticalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                // 实验性标识
                Row(
                    modifier = Modifier
                        .clip(RoundedCornerShape(8.dp))
                        .background(AccentGold.copy(alpha = 0.15f))
                        .padding(horizontal = 12.dp, vertical = 6.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Icon(Icons.Default.BugReport, null, tint = AccentGold, modifier = Modifier.size(16.dp))
                    Spacer(Modifier.width(6.dp))
                    Text("实验性功能 · Experimental", fontSize = 11.sp, color = AccentGold, fontWeight = FontWeight.Medium)
                }

                // 检测状态卡片
                GlassCard(cornerRadius = 16.dp, accentLine = if (fridaDetected) ErrorRed else AccentMint) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Column {
                            Text("Frida 状态", fontSize = 12.sp, color = TextTertiary)
                            Text(
                                if (fridaDetected) "⚠ 检测到 Frida" else "✓ 未检测到",
                                fontSize = 20.sp,
                                fontWeight = FontWeight.Bold,
                                color = if (fridaDetected) ErrorRed else AccentMint
                            )
                        }
                        Icon(
                            Icons.Default.Security,
                            null,
                            tint = if (fridaDetected) ErrorRed else AccentMint,
                            modifier = Modifier.size(32.dp)
                        )
                    }
                }

                // 检测项
                DetectionItem("Frida 二进制", fridaDetected)
                DetectionItem("frida-agent 注入", fridaInMaps)
                DetectionItem("Gum 引擎", gumDetected)
                DetectionItem("默认端口 27042", fridaPort)

                // 开始检测按钮
                Button(
                    onClick = { runScan() },
                    enabled = !scanning,
                    modifier = Modifier.fillMaxWidth().height(48.dp),
                    shape = RoundedCornerShape(12.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = AccentPurple)
                ) {
                    if (scanning) {
                        CircularProgressIndicator(Modifier.size(16.dp), strokeWidth = 2.dp, color = Color.White)
                        Spacer(Modifier.width(8.dp))
                        Text("检测中...")
                    } else {
                        Text("开始 Frida 检测", fontWeight = FontWeight.SemiBold)
                    }
                }

                // 终端日志输出
                if (logText.isNotEmpty()) {
                    Text("检测日志", fontSize = 12.sp, color = TextTertiary, fontWeight = FontWeight.SemiBold)
                    Box(
                        modifier = Modifier
                            .fillMaxWidth()
                            .clip(RoundedCornerShape(12.dp))
                            .background(TermuxBg)
                            .padding(12.dp)
                    ) {
                        Text(
                            logText,
                            color = TermuxGreen,
                            fontSize = 11.sp,
                            fontFamily = FontFamily.Monospace,
                            lineHeight = 16.sp
                        )
                    }
                }

                Spacer(Modifier.height(32.dp))
            }
        }
    }
}

@Composable
private fun DetectionItem(name: String, detected: Boolean) {
    GlassCard(cornerRadius = 12.dp) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(name, fontSize = 13.sp, color = TextPrimary)
            Row(verticalAlignment = Alignment.CenterVertically) {
                Box(
                    Modifier.size(8.dp).clip(RoundedCornerShape(4.dp))
                        .background(if (detected) ErrorRed else AccentMint)
                )
                Spacer(Modifier.width(6.dp))
                Text(
                    if (detected) "检测到" else "正常",
                    fontSize = 12.sp,
                    color = if (detected) ErrorRed else AccentMint,
                    fontWeight = FontWeight.Medium
                )
            }
        }
    }
}
