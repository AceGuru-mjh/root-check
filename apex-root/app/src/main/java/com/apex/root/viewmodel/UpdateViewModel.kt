package com.apex.root.viewmodel

import android.app.Application
import android.util.Log
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.apex.root.data.SettingsRepository
import com.apex.root.data.UpdateChannel
import com.apex.root.data.updater.AppUpdater
import com.apex.root.data.updater.DownloadState
import com.apex.root.data.updater.UpdateChannelPreference
import com.apex.root.data.updater.UpdateCheckResult
import kotlinx.coroutines.CoroutineExceptionHandler
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch

/**
 * 更新检查 UI 状态
 */
sealed class UpdateUiState {
    /** 空闲（未检查） */
    data object Idle : UpdateUiState()
    /** 正在检查 */
    data object Checking : UpdateUiState()
    /** 已是最新版本 */
    data class UpToDate(val currentVersion: String) : UpdateUiState()
    /** 有新版本可用 */
    data class UpdateAvailable(
        val currentVersion: String,
        val newVersion: String,
        val releaseNotes: String,
        val releaseUrl: String,
        val publishedAt: String,
        val apkSizeBytes: Long,
        val isPrerelease: Boolean
    ) : UpdateUiState()
    /** 检查失败 */
    data class CheckFailed(val message: String) : UpdateUiState()
    /** 网络不满足（仅 Wi-Fi 模式下用户在移动网络） */
    data object NetworkNotSatisfied : UpdateUiState()
}

/**
 * 软件更新 ViewModel。
 *
 * 接线关系：
 *  - AppUpdater: 实际的 GitHub API 调用与 APK 下载
 *  - SettingsRepository: 读取更新通道与"仅 Wi-Fi"偏好
 *  - UpdateScreen / SettingsScreen: UI 订阅 [uiState] 与 [downloadState]
 */
class UpdateViewModel(application: Application) : AndroidViewModel(application) {

    private val updater = AppUpdater.getInstance(application)
    private val settingsRepo = SettingsRepository(application)

    private val exceptionHandler = CoroutineExceptionHandler { _, e ->
        Log.e("UpdateViewModel", "Uncaught coroutine exception", e)
    }

    private val _uiState = MutableStateFlow<UpdateUiState>(UpdateUiState.Idle)
    val uiState: StateFlow<UpdateUiState> = _uiState.asStateFlow()

    val downloadState: StateFlow<DownloadState> = updater.downloadState

    // Magisk 模块下载状态（独立于 APK 下载，避免进度互相覆盖）
    val moduleDownloadState: StateFlow<DownloadState> = updater.moduleDownloadState

    /** 缓存最近一次检查到的 release，用于下载 */
    @Volatile
    private var lastAvailableRelease: com.apex.root.data.updater.GitHubRelease? = null

    /** 缓存最近一次查询到的模块 zip 信息 */
    @Volatile
    private var lastModuleZipInfo: com.apex.root.data.updater.ModuleZipInfo? = null

    /** 模块查询状态：null=未查询, false=查询中, true=已完成 */
    private val _moduleCheckState = MutableStateFlow<ModuleCheckState>(ModuleCheckState.Idle)
    val moduleCheckState: StateFlow<ModuleCheckState> = _moduleCheckState.asStateFlow()

    /** 当前应用版本名（如 "1.0.3"） */
    val currentVersion: String by lazy {
        updater.getCurrentVersion().first
    }

    /**
     * 检查更新。
     * @param forceWifiOnlyOverride 强制覆盖 wifiOnly 设置（null 表示用用户设置）
     */
    fun checkForUpdates(forceWifiOnlyOverride: Boolean? = null) {
        if (_uiState.value is UpdateUiState.Checking) return  // 防重复点击

        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            _uiState.value = UpdateUiState.Checking

            try {
                val settings = settingsRepo.load()
                val channel = UpdateChannelPreference.fromValue(settings.updateChannel.value)
                val wifiOnly = forceWifiOnlyOverride ?: settings.wifiOnlyUpdate

                val result = updater.checkForUpdates(channel, wifiOnly)
                _uiState.value = when (result) {
                    is UpdateCheckResult.UpToDate -> UpdateUiState.UpToDate(currentVersion)
                    is UpdateCheckResult.NetworkNotSatisfied -> UpdateUiState.NetworkNotSatisfied
                    is UpdateCheckResult.Error -> UpdateUiState.CheckFailed(result.message)
                    is UpdateCheckResult.Available -> {
                        lastAvailableRelease = result.release
                        UpdateUiState.UpdateAvailable(
                            currentVersion = result.currentVersion,
                            newVersion = result.newVersion,
                            releaseNotes = result.release.releaseBody.ifEmpty { result.release.releaseName },
                            releaseUrl = result.release.htmlUrl,
                            publishedAt = result.release.publishedAt,
                            apkSizeBytes = result.release.apkSize,
                            isPrerelease = result.release.prerelease
                        )
                    }
                }
            } catch (e: Throwable) {
                Log.e("UpdateViewModel", "checkForUpdates failed", e)
                _uiState.value = UpdateUiState.CheckFailed(
                    e.message ?: e.javaClass.simpleName
                )
            }
        }
    }

    /**
     * 下载已检查到的新版本 APK。
     * 调用前请确保 [uiState] 是 [UpdateUiState.UpdateAvailable]。
     */
    fun downloadUpdate() {
        val release = lastAvailableRelease ?: run {
            Log.w("UpdateViewModel", "downloadUpdate called but no release cached")
            return
        }

        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            try {
                val settings = settingsRepo.load()
                updater.downloadApk(release, wifiOnly = settings.wifiOnlyUpdate)
            } catch (e: Throwable) {
                Log.e("UpdateViewModel", "downloadUpdate failed", e)
            }
        }
    }

    /**
     * 取消正在进行的下载。
     */
    fun cancelDownload() {
        updater.cancelDownload()
    }

    /**
     * 安装已下载完成的 APK。
     * @return true 表示安装意图已成功触发
     */
    fun installDownloadedApk(): Boolean {
        val state = downloadState.value
        if (state !is DownloadState.Completed) return false
        return updater.installApk(state.file)
    }

    /**
     * 重置 UI 状态到 Idle（用于关闭更新对话框后清理状态）。
     */
    fun resetState() {
        updater.resetDownloadState()
        _uiState.value = UpdateUiState.Idle
    }

    /**
     * 在浏览器中打开 release 页面（备用方案 — 当 APK 下载失败时）。
     */
    fun openReleaseInBrowser(): Boolean {
        val release = lastAvailableRelease ?: return false
        val intent = android.content.Intent(android.content.Intent.ACTION_VIEW).apply {
            data = android.net.Uri.parse(release.htmlUrl)
            addFlags(android.content.Intent.FLAG_ACTIVITY_NEW_TASK)
        }
        return try {
            getApplication<Application>().startActivity(intent)
            true
        } catch (e: Throwable) {
            Log.e("UpdateViewModel", "openReleaseInBrowser failed", e)
            false
        }
    }

    // ─── Magisk 模块下载 ──────────────────────────

    /**
     * 查询 GitHub Release 中的 Magisk 模块 zip。
     * 查询结果通过 [moduleCheckState] 暴露给 UI。
     */
    fun checkForModuleZip() {
        if (_moduleCheckState.value is ModuleCheckState.Checking) return

        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            _moduleCheckState.value = ModuleCheckState.Checking
            try {
                val moduleInfo = updater.fetchLatestModuleZip()
                if (moduleInfo == null) {
                    _moduleCheckState.value = ModuleCheckState.NotFound
                } else {
                    lastModuleZipInfo = moduleInfo
                    _moduleCheckState.value = ModuleCheckState.Available(moduleInfo)
                }
            } catch (e: Throwable) {
                Log.e("UpdateViewModel", "checkForModuleZip failed", e)
                _moduleCheckState.value = ModuleCheckState.Error(
                    e.message ?: e.javaClass.simpleName
                )
            }
        }
    }

    /**
     * 下载已查询到的 Magisk 模块 zip。
     * 调用前请确保 [moduleCheckState] 是 [ModuleCheckState.Available]。
     */
    fun downloadModuleZip() {
        val moduleInfo = lastModuleZipInfo ?: run {
            Log.w("UpdateViewModel", "downloadModuleZip called but no module info cached")
            return
        }

        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            try {
                val settings = settingsRepo.load()
                updater.downloadModuleZip(moduleInfo, wifiOnly = settings.wifiOnlyUpdate)
            } catch (e: Throwable) {
                Log.e("UpdateViewModel", "downloadModuleZip failed", e)
            }
        }
    }

    /**
     * 取消正在进行的模块下载。
     */
    fun cancelModuleDownload() {
        updater.cancelModuleDownload()
    }

    /**
     * 安装已下载完成的模块 zip（触发 Magisk Manager 刷入流程）。
     * @return true 表示安装意图已成功触发
     */
    fun installDownloadedModuleZip(): Boolean {
        val state = moduleDownloadState.value
        if (state !is DownloadState.Completed) return false
        return updater.installModuleZip(state.file)
    }

    /**
     * 重置模块下载状态。
     */
    fun resetModuleState() {
        updater.resetModuleDownloadState()
        _moduleCheckState.value = ModuleCheckState.Idle
    }
}

/**
 * 模块查询状态
 */
sealed class ModuleCheckState {
    /** 空闲 */
    data object Idle : ModuleCheckState()
    /** 正在查询 */
    data object Checking : ModuleCheckState()
    /** 找到模块 zip */
    data class Available(val info: com.apex.root.data.updater.ModuleZipInfo) : ModuleCheckState()
    /** 未找到模块 zip */
    data object NotFound : ModuleCheckState()
    /** 查询失败 */
    data class Error(val message: String) : ModuleCheckState()
}
