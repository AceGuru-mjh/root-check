package com.rootdetector.ui.compose.screens

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.Memory
import androidx.compose.material.icons.filled.Security
import androidx.compose.material.icons.filled.Storage
import androidx.compose.material.icons.filled.Warning
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

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
        syscallTableStatus = "✅ 正常 (未篡改)",
        loadedModules = listOf("kernel/msm-5.10", "qcom_cmn", "wil6210"),
        selinuxStatus = "Enforcing",
        teeVersion = "TEE 3.1 (QSEE)",
        kallsymsAccessible = true,
        vbarAddress = "0xffffffc010080000",
        securityPatch = "2024-09-01"
    )
) {
    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("内核详细信息") },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = null)
                    }
                }
            )
        }
    ) { padding ->
        LazyColumn(
            modifier = Modifier.fillMaxSize().padding(padding).padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            item {
                InfoCard(
                    icon = Icons.Default.Memory,
                    title = "内核版本",
                    value = kernelInfo.kernelVersion
                )
            }
            item {
                InfoCard(
                    icon = Icons.Default.Storage,
                    title = "架构",
                    value = kernelInfo.architecture
                )
            }
            item {
                InfoCard(
                    icon = Icons.Default.Security,
                    title = "Syscall 表状态",
                    value = kernelInfo.syscallTableStatus
                )
            }
            item {
                Card(
                    modifier = Modifier.fillMaxWidth(),
                    shape = RoundedCornerShape(12.dp),
                    colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant.copy(alpha = 0.4f))
                ) {
                    Column(modifier = Modifier.padding(16.dp)) {
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Icon(Icons.Default.Storage, contentDescription = null, modifier = Modifier.size(20.dp))
                            Spacer(Modifier.width(8.dp))
                            Text("加载的内核模块", style = MaterialTheme.typography.titleSmall, fontWeight = FontWeight.SemiBold)
                        }
                        Spacer(Modifier.height(8.dp))
                        kernelInfo.loadedModules.forEach { module ->
                            Text("  • $module", fontFamily = FontFamily.Monospace, fontSize = 13.sp)
                        }
                    }
                }
            }
            item {
                InfoCard(
                    icon = Icons.Default.Security,
                    title = "SELinux",
                    value = kernelInfo.selinuxStatus
                )
            }
            item {
                InfoCard(
                    icon = Icons.Default.Security,
                    title = "TEE 版本",
                    value = kernelInfo.teeVersion
                )
            }
            item {
                InfoCard(
                    icon = Icons.Default.Warning,
                    title = "VBAR_EL1",
                    value = kernelInfo.vbarAddress
                )
            }
            item {
                InfoCard(
                    icon = Icons.Default.Security,
                    title = "安全补丁",
                    value = kernelInfo.securityPatch
                )
            }
            item {
                InfoCard(
                    icon = Icons.Default.Security,
                    title = "Kallsyms 访问",
                    value = if (kernelInfo.kallsymsAccessible) "✅ 可访问" else "❌ 受限"
                )
            }
            item { Spacer(Modifier.height(32.dp)) }
        }
    }
}

@Composable
private fun InfoCard(
    icon: androidx.compose.ui.graphics.vector.ImageVector,
    title: String,
    value: String
) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        shape = RoundedCornerShape(12.dp),
        colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surfaceVariant.copy(alpha = 0.4f))
    ) {
        Row(
            modifier = Modifier.padding(16.dp).fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Icon(icon, contentDescription = null, modifier = Modifier.size(24.dp))
            Spacer(Modifier.width(12.dp))
            Column(modifier = Modifier.weight(1f)) {
                Text(title, style = MaterialTheme.typography.bodyMedium, color = MaterialTheme.colorScheme.onSurfaceVariant)
                Spacer(Modifier.height(2.dp))
                Text(value, style = MaterialTheme.typography.bodyLarge, fontWeight = FontWeight.Medium, fontFamily = FontFamily.Monospace)
            }
        }
    }
}
