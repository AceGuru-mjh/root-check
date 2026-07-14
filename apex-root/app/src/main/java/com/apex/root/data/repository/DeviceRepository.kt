package com.apex.root.data.repository

import android.os.Build
import com.apex.root.domain.model.BootloaderStatus
import com.apex.root.domain.model.DeviceInfo
import com.apex.root.domain.model.SELinuxMode
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * 设备信息仓库
 * 负责获取和缓存设备硬件/系统信息
 */
class DeviceRepository {
    
    private var cachedDeviceInfo: DeviceInfo? = null
    
    /**
     * 获取设备信息（带缓存）
     */
    suspend fun getDeviceInfo(): DeviceInfo = withContext(Dispatchers.IO) {
        cachedDeviceInfo ?: createDeviceInfo().also { cachedDeviceInfo = it }
    }
    
    /**
     * 刷新设备信息（清除缓存）
     */
    suspend fun refreshDeviceInfo(): DeviceInfo = withContext(Dispatchers.IO) {
        createDeviceInfo().also { cachedDeviceInfo = it }
    }
    
    private fun createDeviceInfo(): DeviceInfo {
        return DeviceInfo(
            brand = Build.BRAND,
            model = Build.MODEL,
            androidVersion = Build.VERSION.RELEASE,
            sdkInt = Build.VERSION.SDK_INT,
            securityPatchLevel = Build.VERSION.SECURITY_PATCH,
            bootloaderStatus = getBootloaderStatus(),
            selinuxMode = getSELinuxMode()
        )
    }
    
    private fun getBootloaderStatus(): BootloaderStatus {
        // TODO: 通过 Native 层检测 bootloader 状态
        // 当前返回 UNKNOWN，后续通过 JNI 实现
        return BootloaderStatus.UNKNOWN
    }
    
    private fun getSELinuxMode(): SELinuxMode {
        return try {
            val process = Runtime.getRuntime().exec("getenforce")
            val output = process.inputStream.bufferedReader().use { it.readText().trim() }
            when (output.lowercase()) {
                "enforcing" -> SELinuxMode.ENFORCING
                "permissive" -> SELinuxMode.PERMISSIVE
                "disabled" -> SELinuxMode.DISABLED
                else -> SELinuxMode.UNKNOWN
            }
        } catch (e: Exception) {
            SELinuxMode.UNKNOWN
        }
    }
}
