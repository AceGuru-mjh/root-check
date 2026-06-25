package com.apex.root.ui

import androidx.compose.ui.test.*
import androidx.compose.ui.test.junit4.createComposeRule
import androidx.test.ext.junit.runners.AndroidJUnit4
import com.apex.root.domain.model.GameModeState
import com.apex.root.domain.guard.model.GuardState
import com.apex.root.domain.model.CureLevel
import com.apex.root.ui.compose.screens.DashboardScreen
import com.apex.root.viewmodel.trusted.ApexUiState
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class DashboardScreenTest {

    @get:Rule
    val composeTestRule = createComposeRule()

    @Test
    fun dashboard_shows_scan_button() {
        composeTestRule.setContent {
            DashboardScreen(
                uiState = ApexUiState(),
                onScan = {},
                onToggleGameMode = {},
                onApplyCure = {},
                onToggleHwid = {},
                onCreateSandbox = {},
                onDestroySandbox = {},
                onRefresh = {}
            )
        }
        composeTestRule.onNodeWithText("快速检测").assertExists()
    }

    @Test
    fun dashboard_shows_deep_scan_button() {
        composeTestRule.setContent {
            DashboardScreen(
                uiState = ApexUiState(),
                onScan = {},
                onDeepScan = {},
                onToggleGameMode = {},
                onApplyCure = {},
                onToggleHwid = {},
                onCreateSandbox = {},
                onDestroySandbox = {},
                onRefresh = {}
            )
        }
        composeTestRule.onNodeWithText("深度检测").assertExists()
    }

    @Test
    fun dashboard_disables_buttons_while_scanning() {
        composeTestRule.setContent {
            DashboardScreen(
                uiState = ApexUiState(isScanning = true),
                onScan = {},
                onDeepScan = {},
                onToggleGameMode = {},
                onApplyCure = {},
                onToggleHwid = {},
                onCreateSandbox = {},
                onDestroySandbox = {},
                onRefresh = {}
            )
        }
        composeTestRule.onNodeWithText("扫描中...").assertExists()
    }

    @Test
    fun dashboard_shows_risk_score_label() {
        composeTestRule.setContent {
            DashboardScreen(
                uiState = ApexUiState(riskScore = 75),
                onScan = {},
                onToggleGameMode = {},
                onApplyCure = {},
                onToggleHwid = {},
                onCreateSandbox = {},
                onDestroySandbox = {},
                onRefresh = {}
            )
        }
        composeTestRule.onNodeWithText("高风险").assertExists()
    }

    @Test
    fun dashboard_shows_scan_result_when_available() {
        composeTestRule.setContent {
            DashboardScreen(
                uiState = ApexUiState(
                    scanResult = "系统属性: 正常\nART: 正常"
                ),
                onScan = {},
                onToggleGameMode = {},
                onApplyCure = {},
                onToggleHwid = {},
                onCreateSandbox = {},
                onDestroySandbox = {},
                onRefresh = {}
            )
        }
        composeTestRule.onNodeWithText("检测结果").assertExists()
    }

    @Test
    fun dashboard_shows_deep_findings_when_present() {
        composeTestRule.setContent {
            DashboardScreen(
                uiState = ApexUiState(
                    scanResult = "检测完成",
                    memFingerprintMask = 1,
                    rwxPageCount = 5
                ),
                onScan = {},
                onToggleGameMode = {},
                onApplyCure = {},
                onToggleHwid = {},
                onCreateSandbox = {},
                onDestroySandbox = {},
                onRefresh = {}
            )
        }
        composeTestRule.onNodeWithText("深度检测发现").assertExists()
    }

    @Test
    fun dashboard_shows_recommendations_when_enabled() {
        composeTestRule.setContent {
            DashboardScreen(
                uiState = ApexUiState(
                    scanResult = "检测完成",
                    showRecommendations = true,
                    recommendations = listOf(
                        com.apex.root.data.FixRecommendation(
                            id = "test",
                            titleZh = "测试建议",
                            titleEn = "Test Recommendation",
                            descriptionZh = "这是一个测试",
                            descriptionEn = "This is a test",
                            stepsZh = listOf("步骤1"),
                            stepsEn = listOf("Step 1"),
                            priority = 5
                        )
                    )
                ),
                onScan = {},
                onToggleGameMode = {},
                onApplyCure = {},
                onToggleHwid = {},
                onCreateSandbox = {},
                onDestroySandbox = {},
                onRefresh = {}
            )
        }
        composeTestRule.onNodeWithText("修复建议").assertExists()
        composeTestRule.onNodeWithText("测试建议").assertExists()
    }

    @Test
    fun game_mode_section_shows_when_active() {
        composeTestRule.setContent {
            DashboardScreen(
                uiState = ApexUiState(
                    gameMode = GameModeState(active = true, hiddenProcesses = 5)
                ),
                onScan = {},
                onToggleGameMode = {},
                onApplyCure = {},
                onToggleHwid = {},
                onCreateSandbox = {},
                onDestroySandbox = {},
                onRefresh = {}
            )
        }
        composeTestRule.onNodeWithText("已激活").assertExists()
        composeTestRule.onNodeWithText("进程隐藏: 5").assertExists()
    }

    @Test
    fun sandbox_section_shows_pid_when_active() {
        composeTestRule.setContent {
            DashboardScreen(
                uiState = ApexUiState(
                    sandboxActive = true,
                    sandboxPid = 1234
                ),
                onScan = {},
                onToggleGameMode = {},
                onApplyCure = {},
                onToggleHwid = {},
                onCreateSandbox = {},
                onDestroySandbox = {},
                onRefresh = {}
            )
        }
        composeTestRule.onNodeWithText("沙箱运行中 (PID: 1234)").assertExists()
    }
}
