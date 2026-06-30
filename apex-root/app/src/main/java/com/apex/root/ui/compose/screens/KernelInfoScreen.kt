package com.apex.root.ui.compose.screens

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
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.input.nestedscroll.nestedScroll
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.apex.root.ui.compose.*

data class KernelInfo(
    val kernelVersion: String,
    val architecture: String,
    val syscallTableStatus: String,
    val loadedModules: List<String>,
    val selinuxStatus: String,
    val teeVersion: String,
    val kallsymsAccessible: Boolean,
    val vbarAddress: String,
    val securityPatch: String
)

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun KernelInfoScreen(
    onBack: () -> Unit = {},
    kernelInfo: KernelInfo = KernelInfo(
        kernelVersion = "5.10.198-android13-8-g3b2c1a0",
        architecture = "ARM64 v8",
        syscallTableStatus = "正常 (未篡改)",
        loadedModules = listOf("kernel/msm-5.10", "qcom_cmn", "wil6210"),
        selinuxStatus = "Enforcing",
        teeVersion = "TEE 3.1 (QSEE)",
        kallsymsAccessible = true,
        vbarAddress = "0xffffffc010080000",
        securityPatch = "2024-09-01"
    )
) {
    val scrollBehavior = TopAppBarDefaults.exitUntilCollapsedScrollBehavior()

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
                        kernelInfo.loadedModules.forEach { module ->
                            Text("  \u2022 $module", fontFamily = FontFamily.Monospace, fontSize = 12.sp, color = TextSecondary)
                            Spacer(Modifier.height(2.dp))
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
                item {
                    InfoCard(
                        icon = Icons.Default.Visibility,
                        title = "Kallsyms 访问",
                        value = if (kernelInfo.kallsymsAccessible) "可访问" else "受限",
                        valueColor = if (kernelInfo.kallsymsAccessible) AccentMint else ErrorRed
                    )
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
