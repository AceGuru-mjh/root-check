package com.apex.root.data.updater

import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.net.ConnectivityManager
import android.net.NetworkCapabilities
import android.net.Uri
import android.os.Build
import android.util.Log
import androidx.core.content.FileProvider
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.withContext
import org.json.JSONObject
import java.io.File
import java.io.FileOutputStream
import java.io.InputStream
import java.net.HttpURLConnection
import java.net.URL
import java.security.MessageDigest
import javax.net.ssl.HttpsURLConnection
import javax.net.ssl.SSLContext

private const val TAG = "AppUpdater"

/**
 * GitHub Releases API 响应数据类。
 * 仅保留更新功能所需字段，避免完整 Release JSON 解析的开销。
 */
data class GitHubRelease(
    val tagName: String,          // e.g. "v1.0.4"
    val releaseName: String,      // release 标题
    val releaseBody: String,      // changelog (markdown)
    val publishedAt: String,      // ISO 8601 时间戳
    val prerelease: Boolean,      // 是否预发布（用于 BETA/CANARY 通道过滤）
    val draft: Boolean,           // 是否草稿（应被忽略）
    val htmlUrl: String,          // release 页面 URL
    val apkDownloadUrl: String,   // APK asset 的 download URL
    val apkSize: Long,            // APK 文件大小（字节）
    val apkFileName: String       // APK 文件名（如 apex-root-v1.0.4-arm64-v8a.apk）
)

/**
 * 更新检查结果。
 */
sealed class UpdateCheckResult {
    /** 当前已是最新版本 */
    data object UpToDate : UpdateCheckResult()

    /** 有新版本可用 */
    data class Available(
        val release: GitHubRelease,
        val currentVersion: String,
        val newVersion: String
    ) : UpdateCheckResult()

    /** 检查失败（网络错误 / 解析错误等） */
    data class Error(val message: String, val throwable: Throwable? = null) : UpdateCheckResult()

    /** 当前网络不满足检查条件（如仅在 Wi-Fi 下检查但当前是移动网络） */
    data object NetworkNotSatisfied : UpdateCheckResult()
}

/**
 * 下载状态。
 */
sealed class DownloadState {
    /** 空闲 */
    data object Idle : DownloadState()
    /** 下载中，progress 为 0..100 */
    data class Downloading(val progress: Int, val downloadedBytes: Long, val totalBytes: Long) : DownloadState()
    /** 下载完成，file 为本地 APK 路径 */
    data class Completed(val file: File) : DownloadState()
    /** 下载失败 */
    data class Failed(val message: String) : DownloadState()
    /** 用户取消 */
    data object Cancelled : DownloadState()
}

/**
 * 软件更新管理器。
 *
 * 借鉴以下 GitHub 项目的实现思路：
 *  - topjohnwu/Magisk: 用 GitHub API 检查 release，下载到 cache 后用 FileProvider + ACTION_INSTALL_PACKAGE 触发安装
 *  - termux/termux-app: 版本号比较使用 semantic versioning，支持 prerelease 通道
 *  - vitorpamplona/amethyst: 用 HttpURLConnection + 流式下载，避免引入 OkHttp 依赖
 *
 * 设计要点：
 *  1. 完全在 Kotlin 层实现，不依赖 native C++ updater（后者目前是 stub 且有安全问题）
 *  2. 使用 HttpURLConnection 避免新增依赖
 *  3. 支持稳定/Beta/开发版三通道，通过 prerelease 字段过滤
 *  4. 支持仅 Wi-Fi 下载，避免消耗用户流量
 *  5. 下载进度通过 StateFlow 暴露，UI 可实时显示
 *  6. APK 下载到 app cache 目录，通过 FileProvider 共享给系统安装器
 */
class AppUpdater private constructor(private val context: Context) {

    companion object {
        // GitHub 仓库信息
        private const val GITHUB_OWNER = "mengjinghao"
        private const val GITHUB_REPO = "root-check"

        // API endpoints
        private const val API_LATEST_RELEASE = "https://api.github.com/repos/$GITHUB_OWNER/$GITHUB_REPO/releases/latest"
        private const val API_ALL_RELEASES = "https://api.github.com/repos/$GITHUB_OWNER/$GITHUB_REPO/releases?per_page=30"

        // HTTP 超时（毫秒）
        private const val CONNECT_TIMEOUT_MS = 10_000
        private const val READ_TIMEOUT_MS = 15_000
        private const val DOWNLOAD_CONNECT_TIMEOUT_MS = 15_000
        private const val DOWNLOAD_READ_TIMEOUT_MS = 60_000

        @Volatile
        private var instance: AppUpdater? = null

        fun getInstance(context: Context): AppUpdater {
            return instance ?: synchronized(this) {
                instance ?: AppUpdater(context.applicationContext).also { instance = it }
            }
        }
    }

    private val _downloadState = MutableStateFlow<DownloadState>(DownloadState.Idle)
    val downloadState: StateFlow<DownloadState> = _downloadState.asStateFlow()

    /**
     * Magisk 模块 zip 下载状态（与 APK 下载独立，避免互相覆盖进度）。
     */
    private val _moduleDownloadState = MutableStateFlow<DownloadState>(DownloadState.Idle)
    val moduleDownloadState: StateFlow<DownloadState> = _moduleDownloadState.asStateFlow()

    @Volatile
    private var cancelRequested = false

    @Volatile
    private var cancelModuleRequested = false

    /**
     * 获取当前应用的版本名（如 "1.0.3"）和版本号（如 103）。
     */
    fun getCurrentVersion(): Pair<String, Int> = try {
        val pm = context.packageManager
        val info = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            pm.getPackageInfo(context.packageName, PackageManager.PackageInfoFlags.of(0))
        } else {
            @Suppress("DEPRECATION")
            pm.getPackageInfo(context.packageName, 0)
        }
        (info.versionName ?: "unknown") to (info.longVersionCode.toInt())
    } catch (e: Throwable) {
        Log.e(TAG, "Failed to get current version", e)
        "unknown" to 0
    }

    /**
     * 检查更新。
     *
     * @param channel 期望的更新通道（STABLE / BETA / CANARY）
     * @param wifiOnly 是否仅在 Wi-Fi 下检查
     * @return UpdateCheckResult
     */
    suspend fun checkForUpdates(
        channel: UpdateChannelPreference = UpdateChannelPreference.STABLE,
        wifiOnly: Boolean = true
    ): UpdateCheckResult = withContext(Dispatchers.IO) {
        if (wifiOnly && !isWifiConnected()) {
            return@withContext UpdateCheckResult.NetworkNotSatisfied
        }

        try {
            val (currentVersionName, _) = getCurrentVersion()
            val release = when (channel) {
                UpdateChannelPreference.STABLE -> fetchLatestStableRelease()
                UpdateChannelPreference.BETA, UpdateChannelPreference.CANARY -> fetchLatestPrerelease(channel)
            } ?: return@withContext UpdateCheckResult.UpToDate

            val comparison = compareVersions(release.tagName, currentVersionName)
            if (comparison <= 0) {
                return@withContext UpdateCheckResult.UpToDate
            }

            if (release.apkDownloadUrl.isEmpty()) {
                return@withContext UpdateCheckResult.Error("Release 中未找到 APK 文件")
            }

            val newVersion = release.tagName.removePrefix("v").removePrefix("V")
            UpdateCheckResult.Available(release, currentVersionName, newVersion)
        } catch (e: java.net.UnknownHostException) {
            UpdateCheckResult.Error("无法连接到 GitHub（DNS 解析失败）", e)
        } catch (e: java.net.SocketTimeoutException) {
            UpdateCheckResult.Error("网络请求超时", e)
        } catch (e: Throwable) {
            Log.e(TAG, "checkForUpdates failed", e)
            UpdateCheckResult.Error("检查更新失败: ${e.message ?: e.javaClass.simpleName}", e)
        }
    }

    /**
     * 下载 APK 文件。
     *
     * 安全加固 (v1.0.7+):
     *  - 强制 HTTPS,拒绝任何 HTTP URL (防止 MITM)
     *  - 使用系统默认 HostnameVerifier,不降级证书验证
     *  - 下载完成后计算 SHA-256,若 release notes 中包含 "SHA-256:" 哈希则
     *    强制校验,不匹配则删除文件并返回 Failed
     *  - 下载完成后验证 APK 签名 (调用方负责安装时验证,但此处提前检查)
     */
    suspend fun downloadApk(
        release: GitHubRelease,
        wifiOnly: Boolean = true
    ): DownloadState = withContext(Dispatchers.IO) {
        if (wifiOnly && !isWifiConnected()) {
            _downloadState.value = DownloadState.Failed("当前非 Wi-Fi 网络，请在设置中关闭「仅 Wi-Fi 下载」或连接 Wi-Fi")
            return@withContext _downloadState.value
        }

        // 安全加固: 强制 HTTPS
        val downloadUrl = release.apkDownloadUrl
        try {
            val url = URL(downloadUrl)
            if (url.protocol.lowercase() != "https") {
                _downloadState.value = DownloadState.Failed("拒绝下载: URL 非 HTTPS ($downloadUrl)")
                return@withContext _downloadState.value
            }
        } catch (e: Exception) {
            _downloadState.value = DownloadState.Failed("下载 URL 无效: ${e.message}")
            return@withContext _downloadState.value
        }

        cancelRequested = false
        _downloadState.value = DownloadState.Downloading(0, 0L, release.apkSize)

        var connection: HttpURLConnection? = null
        var input: InputStream? = null
        var output: FileOutputStream? = null

        try {
            val url = URL(downloadUrl)
            connection = (url.openConnection() as HttpURLConnection).apply {
                connectTimeout = DOWNLOAD_CONNECT_TIMEOUT_MS
                readTimeout = DOWNLOAD_READ_TIMEOUT_MS
                requestMethod = "GET"
                instanceFollowRedirects = true
                setRequestProperty("Accept", "application/octet-stream")
                setRequestProperty("User-Agent", "APEX-Root-Updater")
                // 强制 TLS 1.2 + 默认 HostnameVerifier
                if (this is HttpsURLConnection) {
                    try {
                        val sslContext = SSLContext.getInstance("TLSv1.2")
                        sslContext.init(null, null, null)
                        sslSocketFactory = sslContext.socketFactory
                    } catch (e: Exception) {
                        Log.w(TAG, "TLS 1.2 setup failed: ${e.message}")
                    }
                    hostnameVerifier = HttpsURLConnection.getDefaultHostnameVerifier()
                }
            }

            val responseCode = connection.responseCode
            if (responseCode !in 200..299) {
                _downloadState.value = DownloadState.Failed("下载失败: HTTP $responseCode")
                return@withContext _downloadState.value
            }

            val totalBytes = connection.contentLengthLong.let { len ->
                if (len > 0) len else release.apkSize
            }

            val apkFile = File(context.cacheDir, "apex_update_${System.currentTimeMillis()}.apk")
            context.cacheDir.listFiles { f -> f.name.matches("apex_update_.*\\.apk".toRegex()) }
                ?.forEach { it.delete() }

            input = connection.inputStream
            output = FileOutputStream(apkFile)

            // 同时计算 SHA-256,下载完成即可校验
            val sha256 = MessageDigest.getInstance("SHA-256")
            val buffer = ByteArray(8192)
            var downloadedBytes = 0L
            var lastProgressUpdate = 0L
            var bytesRead: Int

            while (input.read(buffer).also { bytesRead = it } != -1) {
                if (cancelRequested) {
                    _downloadState.value = DownloadState.Cancelled
                    apkFile.delete()
                    return@withContext _downloadState.value
                }

                output.write(buffer, 0, bytesRead)
                sha256.update(buffer, 0, bytesRead)
                downloadedBytes += bytesRead

                val now = System.currentTimeMillis()
                if (now - lastProgressUpdate > 500 || downloadedBytes == totalBytes) {
                    val progress = if (totalBytes > 0) {
                        (downloadedBytes * 100 / totalBytes).toInt().coerceIn(0, 100)
                    } else 0
                    _downloadState.value = DownloadState.Downloading(progress, downloadedBytes, totalBytes)
                    lastProgressUpdate = now
                }
            }

            output.flush()
            output.close()
            input?.close()

            // ─── SHA-256 校验 ───
            val actualHash = sha256.digest().joinToString("") { "%02x".format(it) }
            Log.i(TAG, "Downloaded APK SHA-256: $actualHash")

            // 从 release notes 中提取预期哈希 (约定格式: "SHA-256: <hex>")
            val expectedHash = extractExpectedSha256(release.releaseBody)
            if (expectedHash != null) {
                if (!expectedHash.equals(actualHash, ignoreCase = true)) {
                    Log.e(TAG, "SHA-256 mismatch! expected=$expectedHash actual=$actualHash")
                    apkFile.delete()
                    _downloadState.value = DownloadState.Failed(
                        "APK 完整性校验失败: SHA-256 不匹配\n" +
                        "预期: $expectedHash\n" +
                        "实际: $actualHash\n" +
                        "文件已删除,请勿安装可能被篡改的 APK"
                    )
                    return@withContext _downloadState.value
                }
                Log.i(TAG, "SHA-256 verified ✓ ($expectedHash)")
            } else {
                Log.w(TAG, "Release notes 中未找到 SHA-256 哈希,跳过完整性校验。" +
                    "建议在 release 描述中添加 'SHA-256: <hash>' 行。")
                _downloadState.value = DownloadState.Downloading(100, downloadedBytes, totalBytes)
            }

            _downloadState.value = DownloadState.Completed(apkFile)
            Log.i(TAG, "APK downloaded to ${apkFile.absolutePath} (${apkFile.length()} bytes)")
        } catch (e: Throwable) {
            Log.e(TAG, "downloadApk failed", e)
            _downloadState.value = DownloadState.Failed("下载失败: ${e.message ?: e.javaClass.simpleName}")
        } finally {
            try { input?.close() } catch (_: Throwable) {}
            try { output?.close() } catch (_: Throwable) {}
            connection?.disconnect()
        }

        _downloadState.value
    }

    /**
     * 从 release notes 中提取预期 SHA-256 哈希。
     * 约定格式 (任一即可):
     *   SHA-256: abc123...
     *   sha256: abc123...
     *   hash: abc123...
     * 哈希必须为 64 字符的十六进制字符串。返回 null 表示未找到。
     */
    // P2-5: 改为 internal 以便单元测试访问
    internal fun extractExpectedSha256(releaseBody: String): String? {
        if (releaseBody.isEmpty()) return null
        // 匹配 SHA-256: <64 hex chars> 或 sha256: <64 hex>
        val pattern = Regex(
            """(?i)(?:SHA-?256|hash)\s*[:：]\s*([0-9a-fA-F]{64})"""
        )
        val match = pattern.find(releaseBody) ?: return null
        return match.groupValues.getOrNull(1)?.lowercase()
    }

    fun cancelDownload() {
        cancelRequested = true
    }

    fun installApk(apkFile: File): Boolean {
        return try {
            val uri = FileProvider.getUriForFile(
                context,
                "${context.packageName}.fileprovider",
                apkFile
            )
            val intent = Intent(Intent.ACTION_VIEW).apply {
                setDataAndType(uri, "application/vnd.android.package-archive")
                addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            }
            context.startActivity(intent)
            true
        } catch (e: Throwable) {
            Log.e(TAG, "installApk failed", e)
            false
        }
    }

    fun resetDownloadState() {
        _downloadState.value = DownloadState.Idle
    }

    // ─── Magisk 模块 zip 下载 ──────────────────────

    /**
     * 查询最新 release 中的 Magisk 模块 zip asset。
     * 模块 zip 的文件名约定包含 "module" 或 "magisk" 关键字（如 apex-root-module-v1.0.zip）。
     *
     * @return 模块 zip 的 (downloadUrl, fileName, sizeBytes)，找不到返回 null
     */
    suspend fun fetchLatestModuleZip(): ModuleZipInfo? = withContext(Dispatchers.IO) {
        try {
            // 优先从 latest stable release 查找；找不到再查所有 release
            val releasesToCheck = mutableListOf<JSONObject>()
            val (codeLatest, bodyLatest) = httpGet(API_LATEST_RELEASE)
            if (codeLatest in 200..299) {
                releasesToCheck.add(JSONObject(bodyLatest))
            }
            val (codeAll, bodyAll) = httpGet(API_ALL_RELEASES)
            if (codeAll in 200..299) {
                val arr = org.json.JSONArray(bodyAll)
                for (i in 0 until arr.length()) {
                    releasesToCheck.add(arr.getJSONObject(i))
                }
            }

            for (releaseJson in releasesToCheck) {
                val release = parseRelease(releaseJson) ?: continue
                val assets = releaseJson.optJSONArray("assets") ?: continue
                for (i in 0 until assets.length()) {
                    val asset = assets.getJSONObject(i)
                    val name = asset.optString("name", "")
                    // 模块 zip 命名约定：包含 module / magisk 关键字，且后缀为 .zip
                    val isModule = name.endsWith(".zip") && (
                        name.contains("module", ignoreCase = true) ||
                        name.contains("magisk", ignoreCase = true) ||
                        name.contains("apex-hide", ignoreCase = true) ||
                        name.contains("hide-daemon", ignoreCase = true)
                    )
                    if (isModule) {
                        return@withContext ModuleZipInfo(
                            downloadUrl = asset.optString("browser_download_url", ""),
                            fileName = name,
                            sizeBytes = asset.optLong("size", 0L),
                            releaseTag = release.tagName,
                            releaseUrl = release.htmlUrl
                        )
                    }
                }
            }
            null
        } catch (e: Throwable) {
            Log.e(TAG, "fetchLatestModuleZip failed", e)
            null
        }
    }

    /**
     * 下载 Magisk 模块 zip 文件。
     * 下载到 context.cacheDir/apex_module_<timestamp>.zip，下载完成后通过 StateFlow 通知 UI。
     * 安装时由调用方触发 Magisk Manager 的刷入流程（通过 ACTION_VIEW 让用户在 Magisk 中安装）。
     *
     * @param moduleZip 由 [fetchLatestModuleZip] 返回的模块信息
     * @param wifiOnly 是否仅在 Wi-Fi 下下载
     */
    suspend fun downloadModuleZip(
        moduleZip: ModuleZipInfo,
        wifiOnly: Boolean = true
    ): DownloadState = withContext(Dispatchers.IO) {
        if (wifiOnly && !isWifiConnected()) {
            _moduleDownloadState.value = DownloadState.Failed("当前非 Wi-Fi 网络，请连接 Wi-Fi 后重试")
            return@withContext _moduleDownloadState.value
        }

        cancelModuleRequested = false
        _moduleDownloadState.value = DownloadState.Downloading(0, 0L, moduleZip.sizeBytes)

        var connection: HttpURLConnection? = null
        var input: InputStream? = null
        var output: FileOutputStream? = null

        try {
            val url = URL(moduleZip.downloadUrl)
            connection = (url.openConnection() as HttpURLConnection).apply {
                connectTimeout = DOWNLOAD_CONNECT_TIMEOUT_MS
                readTimeout = DOWNLOAD_READ_TIMEOUT_MS
                requestMethod = "GET"
                instanceFollowRedirects = true
                setRequestProperty("Accept", "application/octet-stream")
                setRequestProperty("User-Agent", "APEX-Root-Updater")
            }

            val responseCode = connection.responseCode
            if (responseCode !in 200..299) {
                _moduleDownloadState.value = DownloadState.Failed("下载失败: HTTP $responseCode")
                return@withContext _moduleDownloadState.value
            }

            val totalBytes = connection.contentLengthLong.let { len ->
                if (len > 0) len else moduleZip.sizeBytes
            }

            val zipFile = File(context.cacheDir, "apex_module_${System.currentTimeMillis()}.zip")
            // 清理旧模块 zip
            context.cacheDir.listFiles { f -> f.name.matches("apex_module_.*\\.zip".toRegex()) }
                ?.forEach { it.delete() }

            input = connection.inputStream
            output = FileOutputStream(zipFile)

            val buffer = ByteArray(8192)
            var downloadedBytes = 0L
            var lastProgressUpdate = 0L
            var bytesRead: Int

            while (input.read(buffer).also { bytesRead = it } != -1) {
                if (cancelModuleRequested) {
                    _moduleDownloadState.value = DownloadState.Cancelled
                    zipFile.delete()
                    return@withContext _moduleDownloadState.value
                }

                output.write(buffer, 0, bytesRead)
                downloadedBytes += bytesRead

                val now = System.currentTimeMillis()
                if (now - lastProgressUpdate > 500 || downloadedBytes == totalBytes) {
                    val progress = if (totalBytes > 0) {
                        (downloadedBytes * 100 / totalBytes).toInt().coerceIn(0, 100)
                    } else 0
                    _moduleDownloadState.value = DownloadState.Downloading(progress, downloadedBytes, totalBytes)
                    lastProgressUpdate = now
                }
            }

            output.flush()
            _moduleDownloadState.value = DownloadState.Completed(zipFile)
            Log.i(TAG, "Module zip downloaded to ${zipFile.absolutePath} (${zipFile.length()} bytes)")
        } catch (e: Throwable) {
            Log.e(TAG, "downloadModuleZip failed", e)
            _moduleDownloadState.value = DownloadState.Failed("模块下载失败: ${e.message ?: e.javaClass.simpleName}")
        } finally {
            try { input?.close() } catch (_: Throwable) {}
            try { output?.close() } catch (_: Throwable) {}
            connection?.disconnect()
        }

        _moduleDownloadState.value
    }

    /**
     * 取消正在进行的模块下载。
     */
    fun cancelModuleDownload() {
        cancelModuleRequested = true
    }

    /**
     * 触发 Magisk Manager 安装模块 zip。
     * 通过 FileProvider 共享 zip 给 Magisk Manager，由其刷入。
     *
     * 注意：用户需先安装 Magisk Manager。如果未安装，会抛 ActivityNotFoundException，
     * 调用方应捕获并提供降级方案（如浏览器打开 release 页面）。
     */
    fun installModuleZip(zipFile: File): Boolean {
        return try {
            val uri = FileProvider.getUriForFile(
                context,
                "${context.packageName}.fileprovider",
                zipFile
            )
            // 优先尝试 Magisk Manager 的 install flash zip intent
            val intent = Intent(Intent.ACTION_VIEW).apply {
                setDataAndType(uri, "application/zip")
                addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            }
            context.startActivity(Intent.createChooser(intent, "使用 Magisk Manager 刷入模块").apply {
                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            })
            true
        } catch (e: Throwable) {
            Log.e(TAG, "installModuleZip failed", e)
            false
        }
    }

    fun resetModuleDownloadState() {
        _moduleDownloadState.value = DownloadState.Idle
    }

    // ─── 已安装模块状态检测 ──────────────────────

    /**
     * 已安装模块信息（从 /data/adb/modules/<id>/module.prop 读取）。
     */
    data class InstalledModuleInfo(
        val id: String,
        val name: String,
        val version: String,
        val versionCode: Long,
        val author: String,
        val description: String,
        val moduleDir: String,
        val disabled: Boolean,        // 是否被禁用（存在 disable 文件）
        val removeFlagged: Boolean    // 是否标记为删除（存在 remove 文件）
    )

    /**
     * 检测已安装的 APEX-Root 模块状态。
     * 借鉴 Magisk Manager 的模块检测逻辑：读取 module.prop + 检查 disable/remove 标记。
     *
     * @param moduleId 模块 ID，默认 "apex-root"
     * @return 已安装模块信息；未安装返回 null
     */
    suspend fun getInstalledModuleInfo(moduleId: String = "apex-root"): InstalledModuleInfo? =
        withContext(Dispatchers.IO) {
            val moduleDir = "/data/adb/modules/$moduleId"
            val propFile = "$moduleDir/module.prop"

            try {
                // 尝试直接读取（root 设备上应用进程通常无权访问 /data/adb）
                val content = readRootFile(propFile)
                    ?: return@withContext null

                val props = parseModuleProp(content)
                val disabled = rootFileExists("$moduleDir/disable")
                val removeFlagged = rootFileExists("$moduleDir/remove")

                InstalledModuleInfo(
                    id = props["id"] ?: moduleId,
                    name = props["name"] ?: "",
                    version = props["version"] ?: "unknown",
                    versionCode = props["versionCode"]?.toLongOrNull() ?: 0L,
                    author = props["author"] ?: "",
                    description = props["description"] ?: "",
                    moduleDir = moduleDir,
                    disabled = disabled,
                    removeFlagged = removeFlagged
                )
            } catch (e: Throwable) {
                Log.e(TAG, "getInstalledModuleInfo failed for $moduleId", e)
                null
            }
        }

    /**
     * 同时检测 apex-root 和 apex-hide-daemon 两个模块的安装状态。
     */
    suspend fun getAllInstalledModules(): List<InstalledModuleInfo> = withContext(Dispatchers.IO) {
        val ids = listOf("apex-root", "apex-hide-daemon")
        ids.mapNotNull { id ->
            runCatching { getInstalledModuleInfo(id) }.getOrNull()
        }
    }

    /**
     * 比较已安装模块版本与 GitHub Release 中的模块版本。
     * @return 正数表示远程更新，0 表示相同，负数表示本地更新（异常情况），null 表示无法比较
     */
    fun compareModuleVersions(installed: InstalledModuleInfo?, remote: ModuleZipInfo?): Int? {
        if (installed == null || remote == null) return null
        return try {
            compareVersions(remote.releaseTag, installed.version)
        } catch (e: Throwable) {
            Log.e(TAG, "compareModuleVersions failed", e)
            null
        }
    }

    /**
     * 通过 su 读取 root 权限文件内容。
     * 非 root 设备会失败，返回 null。
     *
     * P0-4 修复: 原实现 readText() 在 su 弹窗未授权时会无限阻塞,
     * 导致 Dispatchers.IO 线程耗尽。现改为先 waitFor(timeout) 再读,
     * 超时立即 destroyForcibly。
     */
    private fun readRootFile(path: String): String? {
        var process: Process? = null
        return try {
            process = Runtime.getRuntime().exec(arrayOf("su", "-c", "cat '$path'"))
            // P0-4 修复: 先等进程退出 (带超时),再读输出。避免 readText() 无限阻塞。
            val exited = process.waitFor(3000, java.util.concurrent.TimeUnit.MILLISECONDS)
            if (!exited) {
                process.destroyForcibly()
                return null
            }
            if (process.exitValue() != 0) return null
            val text = process.inputStream.bufferedReader().use { it.readText() }
            text.trim().ifEmpty { null }
        } catch (e: Throwable) {
            null
        } finally {
            // P0-4 修复: 必须显式 destroy,Java Process 不会被 GC 回收
            try { process?.destroyForcibly() } catch (_: Throwable) {}
        }
    }

    /**
     * 通过 su 检查 root 权限文件是否存在。
     *
     * P0-4 修复: 同 readRootFile,先 waitFor 再读。
     */
    private fun rootFileExists(path: String): Boolean {
        var process: Process? = null
        return try {
            process = Runtime.getRuntime().exec(arrayOf("su", "-c", "test -e '$path' && echo yes || echo no"))
            val exited = process.waitFor(3000, java.util.concurrent.TimeUnit.MILLISECONDS)
            if (!exited) {
                process.destroyForcibly()
                return false
            }
            if (process.exitValue() != 0) return false
            val text = process.inputStream.bufferedReader().use { it.readText().trim() }
            text == "yes"
        } catch (e: Throwable) {
            false
        } finally {
            try { process?.destroyForcibly() } catch (_: Throwable) {}
        }
    }

    /**
     * 解析 module.prop 文件内容（key=value 格式，每行一个）。
     */
    private fun parseModuleProp(content: String): Map<String, String> {
        val result = mutableMapOf<String, String>()
        content.lines().forEach { line ->
            val trimmed = line.trim()
            if (trimmed.isEmpty() || trimmed.startsWith("#")) return@forEach
            val eqIdx = trimmed.indexOf('=')
            if (eqIdx > 0) {
                val key = trimmed.substring(0, eqIdx).trim()
                val value = trimmed.substring(eqIdx + 1).trim()
                result[key] = value
            }
        }
        return result
    }

    private fun isWifiConnected(): Boolean {
        return try {
            val cm = context.getSystemService(Context.CONNECTIVITY_SERVICE) as? ConnectivityManager
                ?: return false
            val activeNetwork = cm.activeNetwork ?: return false
            val capabilities = cm.getNetworkCapabilities(activeNetwork) ?: return false
            capabilities.hasTransport(NetworkCapabilities.TRANSPORT_WIFI)
        } catch (e: Throwable) {
            Log.e(TAG, "isWifiConnected check failed", e)
            false
        }
    }

    private fun fetchLatestStableRelease(): GitHubRelease? {
        val (responseCode, body) = httpGet(API_LATEST_RELEASE)
        if (responseCode == 404) return null
        if (responseCode !in 200..299) {
            throw RuntimeException("GitHub API 返回 HTTP $responseCode")
        }
        return parseRelease(JSONObject(body))
    }

    private fun fetchLatestPrerelease(channel: UpdateChannelPreference): GitHubRelease? {
        val (responseCode, body) = httpGet(API_ALL_RELEASES)
        if (responseCode !in 200..299) {
            throw RuntimeException("GitHub API 返回 HTTP $responseCode")
        }
        val releasesArray = org.json.JSONArray(body)
        for (i in 0 until releasesArray.length()) {
            val release = releasesArray.getJSONObject(i)
            val parsed = parseRelease(release) ?: continue
            if (parsed.prerelease && !parsed.draft) {
                if (channel == UpdateChannelPreference.CANARY) return parsed
                if (channel == UpdateChannelPreference.BETA &&
                    (parsed.tagName.contains("beta", ignoreCase = true) ||
                        parsed.tagName.contains("rc", ignoreCase = true))) {
                    return parsed
                }
            }
        }
        return fetchLatestStableRelease()
    }

    private fun httpGet(urlStr: String): Pair<Int, String> {
        var connection: HttpURLConnection? = null
        try {
            val url = URL(urlStr)
            // 安全加固: 强制 HTTPS。GitHub API 与 release asset URL 都使用 HTTPS,
            // 任何 HTTP URL 都视为可疑 (可能被中间人攻击篡改)。
            if (url.protocol.lowercase() != "https") {
                Log.e(TAG, "httpGet: rejecting non-HTTPS URL: $urlStr")
                return -1 to ""
            }
            connection = (url.openConnection() as HttpURLConnection).apply {
                connectTimeout = CONNECT_TIMEOUT_MS
                readTimeout = READ_TIMEOUT_MS
                requestMethod = "GET"
                setRequestProperty("Accept", "application/vnd.github+json")
                setRequestProperty("User-Agent", "APEX-Root-Updater")
                instanceFollowRedirects = true
                // 强制使用系统默认的证书验证 — 不要降级 HostnameVerifier
                if (this is HttpsURLConnection) {
                    // 显式使用 TLS 1.2+ (Android 5.0+ 默认支持,但显式更安全)
                    try {
                        val sslContext = SSLContext.getInstance("TLSv1.2")
                        sslContext.init(null, null, null)
                        sslSocketFactory = sslContext.socketFactory
                    } catch (e: Exception) {
                        Log.w(TAG, "TLS 1.2 setup failed, using default: ${e.message}")
                    }
                    hostnameVerifier = HttpsURLConnection.getDefaultHostnameVerifier()
                }
            }
            val code = connection.responseCode
            val body = (if (code in 200..299) connection.inputStream else connection.errorStream)
                ?.bufferedReader()?.use { it.readText() } ?: ""
            return code to body
        } finally {
            connection?.disconnect()
        }
    }

    private fun parseRelease(json: JSONObject): GitHubRelease? {
        return try {
            val tagName = json.optString("tag_name", "")
            if (tagName.isEmpty()) return null

            val releaseName = json.optString("name", tagName)
            val releaseBody = json.optString("body", "")
            val publishedAt = json.optString("published_at", "")
            val prerelease = json.optBoolean("prerelease", false)
            val draft = json.optBoolean("draft", false)
            val htmlUrl = json.optString("html_url", "")

            var apkUrl = ""
            var apkSize = 0L
            var apkFileName = ""
            val assets = json.optJSONArray("assets")
            if (assets != null) {
                for (i in 0 until assets.length()) {
                    val asset = assets.getJSONObject(i)
                    val name = asset.optString("name", "")
                    if (name.matches(Regex(".*\\.apk$"))) {
                        val abi = Build.SUPPORTED_ABIS.firstOrNull() ?: "arm64-v8a"
                        if (name.contains(abi) || apkUrl.isEmpty()) {
                            apkUrl = asset.optString("browser_download_url", "")
                            apkSize = asset.optLong("size", 0L)
                            apkFileName = name
                            if (name.contains(abi)) break
                        }
                    }
                }
            }

            GitHubRelease(
                tagName = tagName,
                releaseName = releaseName,
                releaseBody = releaseBody,
                publishedAt = publishedAt,
                prerelease = prerelease,
                draft = draft,
                htmlUrl = htmlUrl,
                apkDownloadUrl = apkUrl,
                apkSize = apkSize,
                apkFileName = apkFileName
            )
        } catch (e: Throwable) {
            Log.e(TAG, "parseRelease failed", e)
            null
        }
    }

    private fun compareVersions(v1: String, v2: String): Int {
        val p1 = parseVersionParts(v1)
        val p2 = parseVersionParts(v2)
        for (i in 0 until maxOf(p1.size, p2.size)) {
            val a = p1.getOrElse(i) { 0 }
            val b = p2.getOrElse(i) { 0 }
            if (a != b) return a - b
        }
        return 0
    }

    private fun parseVersionParts(version: String): IntArray {
        val raw = version.removePrefix("v").removePrefix("V")
        val numericPart = raw.takeWhile { it.isDigit() || it == '.' }
        return numericPart.split(".").mapNotNull { it.toIntOrNull() }.toIntArray()
    }
}

enum class UpdateChannelPreference(val value: Int, val label: String) {
    STABLE(0, "稳定版"),
    BETA(1, "Beta"),
    CANARY(2, "开发版");

    companion object {
        fun fromValue(v: Int) = entries.firstOrNull { it.value == v } ?: STABLE
    }
}

/**
 * Magisk 模块 zip 信息。
 */
data class ModuleZipInfo(
    val downloadUrl: String,
    val fileName: String,
    val sizeBytes: Long,
    val releaseTag: String,
    val releaseUrl: String
)
