package com.rootdetector.ui.compose.navigation

import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.lifecycle.viewmodel.compose.viewModel
import androidx.navigation.NavHostController
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.rootdetector.ui.MainViewModel
import com.rootdetector.ui.compose.screens.*

object Routes {
    const val DASHBOARD = "dashboard"
    const val REPORT = "report"
    const val HISTORY = "history"
    const val SETTINGS = "settings"
    const val KERNEL_INFO = "kernel_info"
    const val BASELINE = "baseline"
    const val FEATURE_TEST = "feature_test"
    const val TIMING_CHART = "timing_chart"
    const val WHITELIST = "whitelist"
    const val CONFIG = "config"
}

@Composable
fun AppNavigation(
    viewModel: MainViewModel = viewModel(),
    navController: NavHostController = rememberNavController(),
    zh: Boolean = true
) {
    val uiState by viewModel.uiState.collectAsState()

    NavHost(
        navController = navController,
        startDestination = Routes.DASHBOARD
    ) {
        composable(Routes.DASHBOARD) {
            DashboardScreen(
                viewModel = viewModel,
                onNavigateToReport = {
                    if (uiState is com.rootdetector.ui.model.DetectionUiState.Success) {
                        navController.navigate(Routes.REPORT)
                    }
                },
                onNavigateToHistory = { navController.navigate(Routes.HISTORY) },
                onNavigateToSettings = { navController.navigate(Routes.SETTINGS) },
                onNavigateToKernelInfo = { navController.navigate(Routes.KERNEL_INFO) },
                onNavigateToBaseline = { navController.navigate(Routes.BASELINE) },
                onNavigateToFeatureTest = { navController.navigate(Routes.FEATURE_TEST) },
                onNavigateToTimingChart = { navController.navigate(Routes.TIMING_CHART) },
                onNavigateToWhitelist = { navController.navigate(Routes.WHITELIST) },
                onNavigateToConfig = { navController.navigate(Routes.CONFIG) },
                zh = zh
            )
        }

        composable(Routes.REPORT) {
            ReportDetailScreen(
                viewModel = viewModel,
                onBack = { navController.popBackStack() },
                zh = zh
            )
        }

        composable(Routes.HISTORY) {
            HistoryScreen(
                viewModel = viewModel,
                onBack = { navController.popBackStack() },
                onSelectEntry = { navController.popBackStack() },
                zh = zh
            )
        }

        composable(Routes.SETTINGS) {
            SettingsScreen(
                onBack = { navController.popBackStack() },
                zh = zh
            )
        }

        composable(Routes.KERNEL_INFO) {
            KernelInfoScreen(onBack = { navController.popBackStack() })
        }

        composable(Routes.BASELINE) {
            BaselineComparisonScreen(onBack = { navController.popBackStack() })
        }

        composable(Routes.FEATURE_TEST) {
            FeatureTestScreen(onBack = { navController.popBackStack() })
        }

        composable(Routes.TIMING_CHART) {
            TimingChartScreen(onBack = { navController.popBackStack() })
        }

        composable(Routes.WHITELIST) {
            WhitelistScreen(onBack = { navController.popBackStack() })
        }

        composable(Routes.CONFIG) {
            ConfigScreen(
                onBack = { navController.popBackStack() },
                zh = zh
            )
        }
    }
}
