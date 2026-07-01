package com.apex.root.ui.compose.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material.icons.filled.Extension
import androidx.compose.material.icons.filled.Science
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.apex.root.data.jni.NativeBridge
import com.apex.root.ui.compose.*
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * LSPosed / Xposed 模块管理 — 实验性功能
 * 检测已安装的 Xposed 模块及 LSPosed 管理器
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun LSPosedManagerScreen(onBack: () -> Unit) {
    val scope = rememberCoroutineScope()
    val isDark = LocalIsDarkTheme.current
    val scrollBehavior = TopAppBarDefaults.exitUntilCollapsedScrollBehavior()

    var scanning by remember { mutableStateOf(false) }
    var xposedDetected by remember { mutableStateOf(false) }
    var lspdDetected by remember { mutableStateOf(false) }
    var riruDetected by remember { mutableStateOf(false) }
    var zygiskDetected by remember { mutableStateOf(false) }
    var logText by remember { mutableStateOf("") }

    fun addLog(msg: String) {
        val time = java.text.SimpleDateFormat("HH:mm:ss", java.util.Locale.getDefault()).format(java.util.Date())
        logText = logText + "[$time] $msg\n"
    }

    fun runScan() {
        scope.launch(Dispatchers.IO) {
            scanning = true
            logText = ""
            addLog("扫描 Xposed / LSPosed 模块...")

            addLog("检测 Xposed 框架")
            xposedDetected = runCatching { NativeBridge.detectXposedFramework() }.getOrDefault(false)
            addLog(if (xposedDetected) "Xposed 框架存在" else "无 Xposed")

            addLog("检测 LSPosed Manager")
            lspdDetected = runCatching { NativeBridge.detectLSPosedManager() }.getOrDefault(false)
            addLog(if (lspdDetected) "LSPosed Manager 已安装" else "无 LSPosed Manager")

            addLog("检测 Riru 模块")
            riruDetected = runCatching { NativeBridge.detectRiruModules() }.getOrDefault(false)
            addLog(if (riruDetected) "Riru 模块存在" else "无 Riru")

            addLog("检测 Zygisk 模块")
            zygiskDetected = runCatching { NativeBridge.detectZygiskModules() }.getOrDefault(false)
            addLog(if (zygiskDetected) "Zygisk 模块存在" else "无 Zygisk")

            addLog("扫描完成")
            scanning = false
        }
    }

    Scaffold(
        modifier = Modifier.nestedScroll(scrollBehavior.nestedScrollConnection),
        topBar = {
            CollapsibleGlassTopBar(
                title = "模块管理",
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
            fluidColorsDark = PageFluidColors.dashboard,
            fluidColorsLight = PageFluidColors.dashboardLight
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
                    Icon(Icons.Default.Science, null, tint = AccentGold, modifier = Modifier.size(16.dp))
                    Spacer(Modifier.width(6.dp))
                    Text("实验性功能 · Experimental", fontSize = 11.sp, color = AccentGold, fontWeight = FontWeight.Medium)
                }

                // 模块列表
                ModuleCard("Xposed 框架", "de.robv.android.xposed", xposedDetected)
                ModuleCard("LSPosed Manager", "org.lsposed.manager", lspdDetected)
                ModuleCard("Riru 模块", "riru-core / edxp", riruDetected)
                ModuleCard("Zygisk 模块", "zygisk / zygisknext", zygiskDetected)

                // 扫描按钮
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
                        Text("扫描中...")
                    } else {
                        Text("扫描已安装模块", fontWeight = FontWeight.SemiBold)
                    }
                }

                // 终端日志
                if (logText.isNotEmpty()) {
                    Text("扫描日志", fontSize = 12.sp, color = TextTertiary, fontWeight = FontWeight.SemiBold)
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
                            fontFamily = androidx.compose.ui.text.font.FontFamily.Monospace,
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
private fun ModuleCard(name: String, pkg: String, detected: Boolean) {
    GlassCard(cornerRadius = 14.dp, accentLine = if (detected) AccentGold else Color.Transparent) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Box(
                modifier = Modifier
                    .size(40.dp)
                    .clip(RoundedCornerShape(10.dp))
                    .background(if (detected) AccentGold.copy(alpha = 0.15f) else TextTertiary.copy(alpha = 0.1f)),
                contentAlignment = Alignment.Center
            ) {
                Icon(
                    Icons.Default.Extension,
                    null,
                    tint = if (detected) AccentGold else TextTertiary,
                    modifier = Modifier.size(20.dp)
                )
            }
            Spacer(Modifier.width(12.dp))
            Column(modifier = Modifier.weight(1f)) {
                Text(name, fontSize = 14.sp, fontWeight = FontWeight.SemiBold, color = TextPrimary)
                Text(pkg, fontSize = 11.sp, color = TextTertiary, fontFamily = androidx.compose.ui.text.font.FontFamily.Monospace)
            }
            Box(
                Modifier
                    .clip(RoundedCornerShape(6.dp))
                    .background(if (detected) AccentGold.copy(alpha = 0.15f) else Color.Transparent)
                    .padding(horizontal = 8.dp, vertical = 4.dp)
            ) {
                Text(
                    if (detected) "已安装" else "未检测",
                    fontSize = 10.sp,
                    color = if (detected) AccentGold else TextTertiary,
                    fontWeight = FontWeight.Medium
                )
            }
        }
    }
}
