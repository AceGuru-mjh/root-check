package com.apex.root

import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.Scaffold
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.apex.root.ui.screens.dashboard.DashboardScreen
import com.apex.root.ui.screens.settings.SettingsScreen

/**
 * v1.0.4: M3 UI 根 Composable — 导航入口。
 * 连接 DashboardScreen 和 SettingsScreen。
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
                onNavigateToSettings = {
                    navController.navigate("settings")
                }
            )
        }
        composable("settings") {
            SettingsScreen(
                onNavigateBack = {
                    navController.popBackStack()
                }
            )
        }
    }
}
