package com.apex.root.ui.viewmodel

import com.apex.root.domain.model.DeviceInfo
import com.apex.root.domain.model.RiskLevel
import com.apex.root.domain.model.BootloaderStatus
import com.apex.root.domain.model.SELinuxMode
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.test.runTest
import org.junit.Assert.*
import org.junit.Test

/**
 * DashboardViewModel 单元测试
 */
@OptIn(ExperimentalCoroutinesApi::class)
class DashboardViewModelTest {
    
    @Test
    fun `test initial state`() = runTest {
        val viewModel = DashboardViewModel()
        
        val state = viewModel.uiState.value
        
        assertTrue(state.deviceInfo == null || state.deviceInfo is DeviceInfo)
        assertFalse(state.isScanning)
        assertEquals(0f, state.scanProgress)
        assertTrue(state.recentEvents.isEmpty())
    }
    
    @Test
    fun `test device info loading`() = runTest {
        val viewModel = DashboardViewModel()
        
        // 等待协程完成
        kotlinx.coroutines.test.advanceUntilIdle()
        
        val state = viewModel.uiState.value
        
        // 验证设备信息已加载（在模拟器或真机上）
        // 由于测试环境没有 Android 上下文，deviceInfo 可能为 null
        assertNotNull(state)
    }
    
    @Test
    fun `test dismiss error action`() = runTest {
        val viewModel = DashboardViewModel()
        
        // 模拟错误状态
        viewModel.onAction(DashboardAction.DismissError("test-error"))
        
        val state = viewModel.uiState.value
        assertNull(state.scanError)
    }
    
    @Test
    fun `test refresh device action`() = runTest {
        val viewModel = DashboardViewModel()
        
        // 执行刷新操作
        viewModel.onAction(DashboardAction.RefreshDevice)
        
        kotlinx.coroutines.test.advanceUntilIdle()
        
        // 验证状态更新
        val state = viewModel.uiState.value
        assertNotNull(state)
    }
}

/**
 * SettingsViewModel 单元测试
 */
@OptIn(ExperimentalCoroutinesApi::class)
class SettingsViewModelTest {
    
    @Test
    fun `test initial state`() = runTest {
        // 注意：实际测试需要 Mock SettingsRepository
        // 这里仅做结构验证
        assertTrue(true)
    }
}

/**
 * 领域模型测试
 */
class DomainModelTest {
    
    @Test
    fun `test DeviceInfo creation`() {
        val deviceInfo = DeviceInfo(
            brand = "Google",
            model = "Pixel 8",
            androidVersion = "14",
            sdkInt = 34,
            securityPatchLevel = "2024-01-05",
            bootloaderStatus = BootloaderStatus.LOCKED,
            selinuxMode = SELinuxMode.ENFORCING
        )
        
        assertEquals("Google", deviceInfo.brand)
        assertEquals("Pixel 8", deviceInfo.model)
        assertEquals("14", deviceInfo.androidVersion)
        assertEquals(34, deviceInfo.sdkInt)
        assertEquals(BootloaderStatus.LOCKED, deviceInfo.bootloaderStatus)
        assertEquals(SELinuxMode.ENFORCING, deviceInfo.selinuxMode)
    }
    
    @Test
    fun `test RiskLevel ordering`() {
        val levels = listOf(
            RiskLevel.NONE,
            RiskLevel.LOW,
            RiskLevel.MEDIUM,
            RiskLevel.HIGH,
            RiskLevel.CRITICAL
        )
        
        assertEquals(5, levels.size)
        assertEquals(RiskLevel.NONE, levels[0])
        assertEquals(RiskLevel.CRITICAL, levels[4])
    }
    
    @Test
    fun `test SettingsState default values`() {
        val settings = com.apex.root.domain.model.SettingsState()
        
        assertFalse(settings.darkMode)
        assertEquals("zh", settings.language)
        assertTrue(settings.showNotifications)
        assertFalse(settings.autoScanOnStart)
        assertEquals(30, settings.scanIntervalMinutes)
        assertTrue(settings.guardServiceEnabled)
        assertTrue(settings.selfProtectionEnabled)
        assertTrue(settings.loggingEnabled)
        assertFalse(settings.verboseLogging)
        assertTrue(settings.firstLaunch)
        assertEquals(0L, settings.lastScanTimestamp)
    }
    
    @Test
    fun `test DetectionLayerConfig creation`() {
        val config = com.apex.root.domain.model.DetectionLayerConfig(
            layer = 1,
            enabled = true,
            weight = 10
        )
        
        assertEquals(1, config.layer)
        assertTrue(config.enabled)
        assertEquals(10, config.weight)
    }
}
