package com.apex.root.ui.compose

import androidx.compose.animation.core.tween
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.scaleIn
import androidx.compose.animation.scaleOut
import androidx.compose.animation.slideInHorizontally
import androidx.compose.animation.slideOutHorizontally
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.viewmodel.compose.viewModel
import androidx.navigation.NavDestination.Companion.hierarchy
import androidx.navigation.NavGraph.Companion.findStartDestination
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.currentBackStackEntryAsState
import androidx.navigation.compose.rememberNavController
import com.apex.root.data.ThemeMode
import com.apex.root.ui.compose.screens.AlertScreen
import com.apex.root.ui.compose.screens.AboutScreen
import com.apex.root.ui.compose.screens.BaselineComparisonScreen
import com.apex.root.ui.compose.screens.ConfigScreen
import com.apex.root.ui.compose.screens.DashboardScreen
import com.apex.root.ui.compose.screens.FeatureTestScreen
import com.apex.root.ui.compose.screens.FridaConsoleScreen
import com.apex.root.ui.compose.screens.GlassLogViewerScreen
import com.apex.root.ui.compose.screens.GlassPermissionGuideScreen
import com.apex.root.ui.compose.screens.HideModeScreen
import com.apex.root.ui.compose.screens.HistoryScreen
import com.apex.root.ui.compose.screens.KernelInfoScreen
import com.apex.root.ui.compose.screens.LSPosedManagerScreen
import com.apex.root.ui.compose.screens.ReportScreen
import com.apex.root.ui.compose.screens.SettingsScreen
import com.apex.root.ui.compose.screens.SplashScreen
import com.apex.root.ui.compose.screens.TimingChartScreen
import com.apex.root.ui.compose.screens.WhitelistScreen
import com.apex.root.viewmodel.SettingsViewModel
import com.apex.root.viewmodel.trusted.ApexViewModel
import com.apex.root.viewmodel.trusted.ScanViewModel

sealed class NavItem(val route: String, val label: String, val icon: ImageVector) {
    data object Dashboard : NavItem("dashboard", "仪表盘", Icons.Default.Home)
    data object Report : NavItem("report", "报告", Icons.Default.Description)
    data object Alert : NavItem("alert", "警报", Icons.Default.Notifications)
    data object Settings : NavItem("settings", "设置", Icons.Default.Settings)
}

private val mainRoutes = listOf("dashboard", "report", "alert", "settings")

@Composable
fun AppNavigation(
    apexViewModel: ApexViewModel = viewModel(),
    scanViewModel: ScanViewModel = viewModel(),
    settingsViewModel: SettingsViewModel = viewModel()
) {
    var showSplash by remember { mutableStateOf(true) }
    val settings by settingsViewModel.settings.collectAsState()
    val isDark = when (settings.themeMode) {
        ThemeMode.DARK -> true
        ThemeMode.LIGHT -> false
        ThemeMode.SYSTEM -> isSystemInDarkTheme()
    }

    ApexRootTheme(darkTheme = isDark) {
        if (showSplash) {
            SplashScreen(onSplashComplete = { showSplash = false })
        } else if (apexViewModel.uiState.value.isFirstLaunch) {
            GlassPermissionGuideScreen(
                onFinished = {
                    apexViewModel.completePermissionGuide()
                }
            )
        } else {
            MainApp(isDark, settingsViewModel, apexViewModel, scanViewModel)
        }
    }
}

@Composable
private fun MainApp(
    isDark: Boolean,
    settingsViewModel: SettingsViewModel,
    apexViewModel: ApexViewModel,
    scanViewModel: ScanViewModel
) {
    val navController = rememberNavController()
    val uiState by apexViewModel.uiState.collectAsState()
    val context = LocalContext.current

    val navItems = listOf(
        NavItem.Dashboard, NavItem.Report,
        NavItem.Alert, NavItem.Settings
    )

    Box(modifier = Modifier.fillMaxSize()) {
        Scaffold(
            containerColor = Color.Transparent,
            bottomBar = {
                val navBackStackEntry by navController.currentBackStackEntryAsState()
                val currentDestination = navBackStackEntry?.destination
                val currentRoute = currentDestination?.route

                if (currentRoute in mainRoutes) {
                    NavigationBar(
                        containerColor = Color.Transparent,
                        tonalElevation = 0.dp
                    ) {
                        Box(
                            modifier = Modifier
                                .fillMaxWidth()
                                .clip(RoundedCornerShape(topStart = 24.dp, topEnd = 24.dp))
                                .liquidGlass(
                                    cornerRadius = 24.dp,
                                    baseColor = if (isDark) DeepSurface.copy(alpha = 0.45f) else LightSurface.copy(alpha = 0.70f)
                                )
                                .padding(top = 4.dp, bottom = 4.dp)
                        ) {
                            Row(
                                modifier = Modifier.fillMaxWidth(),
                                horizontalArrangement = Arrangement.SpaceEvenly
                            ) {
                                navItems.forEach { item ->
                                    val selected = currentDestination?.hierarchy?.any { it.route == item.route } == true
                                    NavigationBarItem(
                                        icon = {
                                            Box(Modifier.size(24.dp), contentAlignment = Alignment.Center) {
                                                Icon(item.icon, item.label,
                                                    Modifier.size(if (selected) 22.dp else 20.dp))
                                            }
                                        },
                                        label = {
                                            Text(item.label, fontSize = 10.sp,
                                                fontWeight = if (selected) FontWeight.SemiBold else FontWeight.Normal,
                                                letterSpacing = 0.3.sp)
                                        },
                                        selected = selected,
                                        onClick = {
                                            navController.navigate(item.route) {
                                                popUpTo(navController.graph.findStartDestination().id) { saveState = true }
                                                launchSingleTop = true
                                                restoreState = true
                                            }
                                        },
                                        colors = NavigationBarItemDefaults.colors(
                                            selectedIconColor = AccentPurple,
                                            selectedTextColor = AccentPurple,
                                            unselectedIconColor = TextTertiary,
                                            unselectedTextColor = TextTertiary,
                                            indicatorColor = AccentPurple.copy(alpha = 0.12f)
                                        )
                                    )
                                }
                            }
                        }
                    }
                }
            }
        ) { innerPadding ->
            NavHost(
                navController = navController,
                startDestination = NavItem.Dashboard.route,
                modifier = Modifier.padding(innerPadding)
            ) {
                composable(
                    route = NavItem.Dashboard.route,
                    enterTransition = { scaleIn(initialScale = 0.85f, animationSpec = tween(450)) + fadeIn(animationSpec = tween(450)) },
                    exitTransition = { scaleOut(targetScale = 1.15f, animationSpec = tween(450)) + fadeOut(animationSpec = tween(450)) },
                    popEnterTransition = { scaleIn(initialScale = 0.85f, animationSpec = tween(450)) + fadeIn(animationSpec = tween(450)) },
                    popExitTransition = { scaleOut(targetScale = 1.15f, animationSpec = tween(450)) + fadeOut(animationSpec = tween(450)) }
                ) {
                    DashboardScreen(
                        uiState = uiState,
                        onScan = { apexViewModel.runScan() },
                        onDeepScan = { apexViewModel.runDeepScan() },
                        onToggleGameMode = { apexViewModel.toggleGameMode() },
                        onApplyCure = { level -> apexViewModel.applyCure(level) },
                        onToggleHwid = { apexViewModel.toggleHwidSpoof() },
                        onCreateSandbox = { name -> apexViewModel.createSandbox(name) },
                        onDestroySandbox = { apexViewModel.destroySandbox() },
                        onRefresh = { apexViewModel.refreshState() },
                        onShowRecommendations = { apexViewModel.showFixRecommendations() },
                        onExportReport = { apexViewModel.exportReport(context.applicationContext) },
                        onDismissRecommendations = { apexViewModel.dismissRecommendations() },
                        onNavigateToHistory = { navController.navigate("history") },
                        onNavigateToKernelInfo = { navController.navigate("kernel_info") },
                        onNavigateToBaseline = { navController.navigate("baseline") },
                        onNavigateToFeatureTest = { navController.navigate("feature_test") },
                        onNavigateToTimingChart = { navController.navigate("timing_chart") },
                        onNavigateToWhitelist = { navController.navigate("whitelist") },
                        onNavigateToConfig = { navController.navigate("config") },
                        onNavigateToHideMode = { navController.navigate("hide_mode") },
                        onNavigateToAbout = { navController.navigate("about") },
                        onNavigateToFrida = { navController.navigate("frida_console") },
                        onNavigateToLSPosed = { navController.navigate("lsposed_manager") },
                        apexViewModel = apexViewModel
                    )
                }
                composable(
                    route = NavItem.Report.route,
                    enterTransition = { scaleIn(initialScale = 0.80f, animationSpec = tween(350)) + fadeIn(animationSpec = tween(350)) },
                    exitTransition = { scaleOut(targetScale = 1.20f, animationSpec = tween(350)) + fadeOut(animationSpec = tween(350)) },
                    popEnterTransition = { scaleIn(initialScale = 0.80f, animationSpec = tween(350)) + fadeIn(animationSpec = tween(350)) },
                    popExitTransition = { scaleOut(targetScale = 1.20f, animationSpec = tween(350)) + fadeOut(animationSpec = tween(350)) }
                ) {
                    ReportScreen(
                        viewModel = scanViewModel,
                        onBack = { navController.popBackStack(NavItem.Dashboard.route, false) }
                    )
                }
                composable(
                    route = NavItem.Alert.route,
                    enterTransition = { scaleIn(initialScale = 0.75f, animationSpec = tween(500)) + fadeIn(animationSpec = tween(500)) },
                    exitTransition = { scaleOut(targetScale = 1.25f, animationSpec = tween(500)) + fadeOut(animationSpec = tween(500)) },
                    popEnterTransition = { scaleIn(initialScale = 0.75f, animationSpec = tween(500)) + fadeIn(animationSpec = tween(500)) },
                    popExitTransition = { scaleOut(targetScale = 1.25f, animationSpec = tween(500)) + fadeOut(animationSpec = tween(500)) }
                ) {
                    AlertScreen(
                        viewModel = scanViewModel,
                        onBack = { navController.popBackStack(NavItem.Dashboard.route, false) }
                    )
                }
                composable(
                    route = NavItem.Settings.route,
                    enterTransition = { scaleIn(initialScale = 0.90f, animationSpec = tween(300)) + fadeIn(animationSpec = tween(300)) },
                    exitTransition = { scaleOut(targetScale = 1.10f, animationSpec = tween(300)) + fadeOut(animationSpec = tween(300)) },
                    popEnterTransition = { scaleIn(initialScale = 0.90f, animationSpec = tween(300)) + fadeIn(animationSpec = tween(300)) },
                    popExitTransition = { scaleOut(targetScale = 1.10f, animationSpec = tween(300)) + fadeOut(animationSpec = tween(300)) }
                ) {
                    SettingsScreen(
                        viewModel = settingsViewModel,
                        apexViewModel = apexViewModel,
                        onNavigateToLogs = { navController.navigate("log_viewer") },
                        onBack = { navController.popBackStack() }
                    )
                }
                composable(
                    route = "log_viewer",
                    enterTransition = { slideInHorizontally(initialOffsetX = { it }) + fadeIn(tween(300)) },
                    exitTransition = { slideOutHorizontally(targetOffsetX = { it }) + fadeOut(tween(300)) }
                ) {
                    GlassLogViewerScreen(
                        viewModel = apexViewModel,
                        onBack = { navController.popBackStack() }
                    )
                }

                // ── New sub‑screens ──
                composable(
                    route = "history",
                    enterTransition = { slideInHorizontally(initialOffsetX = { it }) + fadeIn(tween(300)) },
                    exitTransition = { slideOutHorizontally(targetOffsetX = { it }) + fadeOut(tween(300)) }
                ) {
                    HistoryScreen(
                        onBack = { navController.popBackStack() }
                    )
                }
                composable(
                    route = "kernel_info",
                    enterTransition = { slideInHorizontally(initialOffsetX = { it }) + fadeIn(tween(300)) },
                    exitTransition = { slideOutHorizontally(targetOffsetX = { it }) + fadeOut(tween(300)) }
                ) {
                    KernelInfoScreen(
                        onBack = { navController.popBackStack() }
                    )
                }
                composable(
                    route = "baseline",
                    enterTransition = { slideInHorizontally(initialOffsetX = { it }) + fadeIn(tween(300)) },
                    exitTransition = { slideOutHorizontally(targetOffsetX = { it }) + fadeOut(tween(300)) }
                ) {
                    BaselineComparisonScreen(
                        onBack = { navController.popBackStack() }
                    )
                }
                composable(
                    route = "feature_test",
                    enterTransition = { slideInHorizontally(initialOffsetX = { it }) + fadeIn(tween(300)) },
                    exitTransition = { slideOutHorizontally(targetOffsetX = { it }) + fadeOut(tween(300)) }
                ) {
                    FeatureTestScreen(
                        onBack = { navController.popBackStack() }
                    )
                }
                composable(
                    route = "timing_chart",
                    enterTransition = { slideInHorizontally(initialOffsetX = { it }) + fadeIn(tween(300)) },
                    exitTransition = { slideOutHorizontally(targetOffsetX = { it }) + fadeOut(tween(300)) }
                ) {
                    TimingChartScreen(
                        onBack = { navController.popBackStack() }
                    )
                }
                composable(
                    route = "whitelist",
                    enterTransition = { slideInHorizontally(initialOffsetX = { it }) + fadeIn(tween(300)) },
                    exitTransition = { slideOutHorizontally(targetOffsetX = { it }) + fadeOut(tween(300)) }
                ) {
                    WhitelistScreen(
                        onBack = { navController.popBackStack() }
                    )
                }
                composable(
                    route = "config",
                    enterTransition = { slideInHorizontally(initialOffsetX = { it }) + fadeIn(tween(300)) },
                    exitTransition = { slideOutHorizontally(targetOffsetX = { it }) + fadeOut(tween(300)) }
                ) {
                    ConfigScreen(
                        onBack = { navController.popBackStack() }
                    )
                }
                composable(
                    route = "hide_mode",
                    enterTransition = { slideInHorizontally(initialOffsetX = { it }) + fadeIn(tween(300)) },
                    exitTransition = { slideOutHorizontally(targetOffsetX = { it }) + fadeOut(tween(300)) }
                ) {
                    HideModeScreen(
                        onBack = { navController.popBackStack() }
                    )
                }
                composable(
                    route = "about",
                    enterTransition = { slideInHorizontally(initialOffsetX = { it }) + fadeIn(tween(300)) },
                    exitTransition = { slideOutHorizontally(targetOffsetX = { it }) + fadeOut(tween(300)) }
                ) {
                    AboutScreen(
                        onBack = { navController.popBackStack() }
                    )
                }
                composable(
                    route = "frida_console",
                    enterTransition = { slideInHorizontally(initialOffsetX = { it }) + fadeIn(tween(300)) },
                    exitTransition = { slideOutHorizontally(targetOffsetX = { it }) + fadeOut(tween(300)) }
                ) {
                    FridaConsoleScreen(
                        onBack = { navController.popBackStack() }
                    )
                }
                composable(
                    route = "lsposed_manager",
                    enterTransition = { slideInHorizontally(initialOffsetX = { it }) + fadeIn(tween(300)) },
                    exitTransition = { slideOutHorizontally(targetOffsetX = { it }) + fadeOut(tween(300)) }
                ) {
                    LSPosedManagerScreen(
                        onBack = { navController.popBackStack() }
                    )
                }
            }
        }
    }
}
