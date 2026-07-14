package com.apex.root.ui.screens.dashboard

import androidx.compose.ui.test.junit4.createComposeRule
import androidx.compose.ui.test.onNodeWithContentDescription
import androidx.compose.ui.test.onNodeWithTag
import androidx.compose.ui.test.performClick
import com.apex.root.domain.model.DeviceInfo
import com.apex.root.domain.model.RiskLevel
import com.apex.root.ui.theme.ApexRootTheme
import org.junit.Rule
import org.junit.Test

/**
 * DashboardScreen 页面测试
 */
class DashboardScreenTest {
    
    @get:Rule
    val composeTestRule = createComposeRule()
    
    @Test
    fun dashboard_displaysDeviceSummary() {
        val deviceInfo = DeviceInfo(
            brand = "Google",
            model = "Pixel 8",
            androidVersion = "14",
            sdkInt = 34,
            securityPatchLevel = "2024-01-05"
        )
        
        val state = DashboardUiState(
            deviceInfo = deviceInfo,
            isScanning = false,
            scanProgress = 0f,
            lastScanResult = null,
            recentEvents = emptyList()
        )
        
        composeTestRule.setContent {
            ApexRootTheme {
                DashboardContent(
                    state = state,
                    onAction = {},
                    onNavigateToSettings = {}
                )
            }
        }
        
        // 验证设备信息显示
        composeTestRule.onNodeWithText("Google Pixel 8")
            .assertExists()
        composeTestRule.onNodeWithText("Android 14 (API 34)")
            .assertExists()
    }
    
    @Test
    fun dashboard_settingsButton_clickable() {
        var navigateCalled = false
        
        composeTestRule.setContent {
            ApexRootTheme {
                DashboardContent(
                    state = DashboardUiState(),
                    onAction = {},
                    onNavigateToSettings = { navigateCalled = true }
                )
            }
        }
        
        // 点击设置按钮
        composeTestRule.onNodeWithContentDescription("设置")
            .performClick()
        
        assert(navigateCalled) { "Navigation to settings should be triggered" }
    }
    
    @Test
    fun dashboard_scanCard_displaysCorrectly() {
        val state = DashboardUiState(
            isScanning = false,
            scanProgress = 0f
        )
        
        composeTestRule.setContent {
            ApexRootTheme {
                DashboardContent(
                    state = state,
                    onAction = {},
                    onNavigateToSettings = {}
                )
            }
        }
        
        // 验证扫描卡片显示
        composeTestRule.onNodeWithTag("quickScanCard")
            .assertExists()
    }
}
