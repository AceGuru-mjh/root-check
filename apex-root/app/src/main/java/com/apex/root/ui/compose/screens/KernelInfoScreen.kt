package com.apex.root.ui.compose.screens

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.apex.root.ui.compose.*
import kotlinx.coroutines.withContext
import java.io.File

data class KernelInfo(
    val kernelVersion: String,
    val architecture: String,
    val syscallTableStatus: String,
    val loadedModules: List<String>,
    val selinuxStatus: String,
    val teeVersion: String,
    // kallsymsAccessible removed — was a Ring0 /proc/kallsyms probe.
    // Detection path is now Ring3-only (see layer5_sidechannel.cpp).
    val vbarAddress: String,
    val securityPatch: String
)

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun KernelInfoScreen(onBack: () -> Unit = {}) {
    val scrollBehavior = TopAppBarDefaults.exitUntilCollapsedScrollBehavior()

    // 修复：原 remember { readKernelInfoFromDevice() } 在主线程执行文件读取 + exec(getenforce)，
    // 慢设备上会触发 StrictMode penaltyDeath 或 ANR。改为 LaunchedEffect + IO 线程异步加载。
    var kernelInfo by remember { mutableStateOf<KernelInfo?>(null) }
    LaunchedEffect(Unit) {
        kernelInfo = withContext(kotlinx.coroutines.Dispatchers.IO) { readKernelInfoFromDevice() }
    }
    val info = kernelInfo

    Scaffold(
        modifier = Modifier.nestedScroll(scrollBehavior.nestedScrollConnection),
        containerColor = Color.Transparent,
        topBar = {
            CollapsibleGlassTopBar(
                title = "内核详细信息",
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
            LazyColumn(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(padding)
                    .padding(16.dp),
                verticalArrangement = Arrangement.spacedBy(12.dp),
                contentPadding = PaddingValues(vertical = 8.dp)
            ) {
                item { SectionHeader(title = "系统内核") }

                if (info == null) {
                    item {
                        Box(
                            modifier = Modifier.fillMaxWidth().padding(vertical = 48.dp),
                            contentAlignment = Alignment.Center
                        ) {
                            CircularProgressIndicator(
                                modifier = Modifier.size(28.dp),
                                strokeWidth = 2.5.dp,
                                color = AccentPurple
                            )
                        }
                    }
                } else {
                    val kernelInfo = info
                    item {
                        InfoCard(icon = Icons.Default.Memory, title = "内核版本", value = kernelInfo.kernelVersion)
                    }
                    item {
                        InfoCard(icon = Icons.Default.Storage, title = "架构", value = kernelInfo.architecture)
                    }
                    item {
                        InfoCard(
                            icon = Icons.Default.Security,
                            title = "Syscall 表状态",
                            value = kernelInfo.syscallTableStatus,
                            valueColor = AccentMint
                        )
                    }
                    item {
                        GlassCard(cornerRadius = 16.dp) {
                            Row(verticalAlignment = Alignment.CenterVertically) {
                                GlassIconBox(icon = Icons.Default.Storage, accentColor = AccentBlue, size = 32.dp, iconSize = 16.dp)
                                Spacer(Modifier.width(12.dp))
                                Text("加载的内核模块", fontWeight = FontWeight.SemiBold, fontSize = 13.sp, color = TextPrimary)
                            }
                            Spacer(Modifier.height(10.dp))
                            if (kernelInfo.loadedModules.isEmpty()) {
                                Text("  \u2022 无（或不可读）", fontFamily = FontFamily.Monospace, fontSize = 12.sp, color = TextSecondary)
                            } else {
                                kernelInfo.loadedModules.forEach { module ->
                                    Text("  \u2022 $module", fontFamily = FontFamily.Monospace, fontSize = 12.sp, color = TextSecondary)
                                    Spacer(Modifier.height(2.dp))
                                }
                            }
                        }
                    }
                    item {
                        InfoCard(icon = Icons.Default.Security, title = "SELinux", value = kernelInfo.selinuxStatus)
                    }
                    item {
                        InfoCard(icon = Icons.Default.Security, title = "TEE 版本", value = kernelInfo.teeVersion)
                    }
                    item {
                        InfoCard(icon = Icons.Default.Code, title = "VBAR_EL1", value = kernelInfo.vbarAddress, mono = true)
                    }
                    item {
                        InfoCard(icon = Icons.Default.Shield, title = "安全补丁", value = kernelInfo.securityPatch)
                    }
                    // Kallsyms 访问 card removed — Ring0 probe, see KernelInfo data class.
                }

                item { Spacer(Modifier.height(32.dp)) }
            }
        }
    }
}

@Composable
private fun InfoCard(
    icon: ImageVector,
    title: String,
    value: String,
    valueColor: Color = TextPrimary,
    mono: Boolean = false
) {
    GlassCard(cornerRadius = 16.dp) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically
        ) {
            GlassIconBox(icon = icon, accentColor = AccentBlue, size = 36.dp, iconSize = 18.dp)
            Spacer(Modifier.width(14.dp))
            Column(modifier = Modifier.weight(1f)) {
                Text(title, fontSize = 11.sp, color = TextTertiary)
                Spacer(Modifier.height(2.dp))
                Text(
                    value,
                    fontSize = 14.sp,
                    fontWeight = FontWeight.Medium,
                    fontFamily = if (mono) FontFamily.Monospace else FontFamily.Default,
                    color = valueColor
                )
            }
        }
    }
}

private fun readKernelInfoFromDevice(): KernelInfo = runCatching {
    val kernelVersion = runCatching {
        File("/proc/version").bufferedReader().readText().trim()
    }.getOrDefault("/proc/version 不可读")

    val props = readBuildProp()
    val arch = props["ro.product.cpu.abi"]
        ?: props["ro.arch"]
        ?: runCatching { System.getProperty("os.arch") }.getOrDefault("未知")

    val selinux = runCatching {
        val p = Runtime.getRuntime().exec(arrayOf("getenforce"))
        p.inputStream.bufferedReader().readText().trim()
    }.getOrDefault("Unknown")

    val securityPatch = props["ro.build.version.security_patch"]
        ?: props["ro.build.version.security_update"]
        ?: "未知"

    val loadedModules = readLoadedModules()
    // kallsymsAccessible check removed (Ring0 /proc/kallsyms probe).

    KernelInfo(
        kernelVersion = kernelVersion,
        architecture = arch,
        syscallTableStatus = "需 root 才能检测",
        loadedModules = loadedModules,
        selinuxStatus = selinux,
        teeVersion = "需 root 才能读取",
        vbarAddress = "需 root 才能读取",
        securityPatch = securityPatch
    )
}.getOrElse {
    KernelInfo(
        kernelVersion = "读取失败",
        architecture = "未知",
        syscallTableStatus = "未知",
        loadedModules = emptyList(),
        selinuxStatus = "未知",
        teeVersion = "未知",
        vbarAddress = "未知",
        securityPatch = "未知"
    )
}

private fun readBuildProp(): Map<String, String> {
    val props = mutableMapOf<String, String>()
    listOf("/system/build.prop", "/vendor/build.prop", "/product/build.prop").forEach { path ->
        runCatching {
            File(path).bufferedReader().useLines { lines ->
                lines.forEach { line ->
                    val trimmed = line.trim()
                    if (trimmed.isNotEmpty() && !trimmed.startsWith("#") && trimmed.contains("=")) {
                        val idx = trimmed.indexOf('=')
                        val k = trimmed.substring(0, idx).trim()
                        val v = trimmed.substring(idx + 1).trim()
                        if (k.isNotEmpty() && !props.containsKey(k)) props[k] = v
                    }
                }
            }
        }
    }
    return props
}

private fun readLoadedModules(): List<String> = runCatching {
    File("/proc/modules").bufferedReader().useLines { lines ->
        lines.mapNotNull { line ->
            val name = line.substringBefore(' ').trim()
            if (name.isNotEmpty()) name else null
        }.take(20).toList()
    }
}.getOrDefault(emptyList())

// checkKallsymsAccessible() removed — Ring0 /proc/kallsyms probe.
// Syscall-tamper detection is now done in Ring3 via
// NativeBridge.detectSyscallResultInconsistency() (layer5_sidechannel.cpp).
