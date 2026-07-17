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
 * v1.0.5: M3 UI 根 Composable — 导航入口。
 * Dashboard ↔ ScanResult ↔ Settings
 */
@Composable
fun ApexRootApp() {
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
