package com.apex.root

import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.apex.root.ui.screens.dashboard.DashboardScreen
import com.apex.root.ui.screens.scanresult.ScanResultScreen
import com.apex.root.ui.screens.settings.SettingsScreen
import com.apex.root.ui.theme.ApexRootTheme

/**
 * v1.1.1: M3 UI 根 Composable — 导航入口 (原 ApexRootApp, 已重命名避免与 Application 子类冲突)。
 * Dashboard ↔ ScanResult ↔ Settings
 *
 * 修复 P0-K1: 之前 `@Composable fun ApexRootApp()` 与 AndroidManifest 的
 * `android:name=".ApexRootApp"` (期望 Application 子类) 命名冲突, 导致启动崩溃。
 * 现在 Application 子类为 [ApexRootApplication], 本函数仅负责 Compose 导航。
 */
@Composable
fun ApexRootNavHost() {
    val navController = rememberNavController()

    NavHost(
        navController = navController,
        startDestination = "dashboard",
        modifier = Modifier.fillMaxSize()
    ) {
        composable("dashboard") {
            DashboardScreen(
                onNavigateToSettings = { navController.navigate("settings") },
                onNavigateToScanResult = { navController.navigate("scanresult") }
            )
        }
        composable("scanresult") {
            ScanResultScreen(
                onNavigateBack = { navController.popBackStack() }
            )
        }
        composable("settings") {
            SettingsScreen(
                onNavigateBack = { navController.popBackStack() }
            )
        }
    }
}
