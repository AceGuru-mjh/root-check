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

    @Volatile
    private var cancelRequested = false

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
     */
    suspend fun downloadApk(
        release: GitHubRelease,
        wifiOnly: Boolean = true
    ): DownloadState = withContext(Dispatchers.IO) {
        if (wifiOnly && !isWifiConnected()) {
            _downloadState.value = DownloadState.Failed("当前非 Wi-Fi 网络，请在设置中关闭「仅 Wi-Fi 下载」或连接 Wi-Fi")
            return@withContext _downloadState.value
        }

        cancelRequested = false
        _downloadState.value = DownloadState.Downloading(0, 0L, release.apkSize)

        var connection: HttpURLConnection? = null
        var input: InputStream? = null
        var output: FileOutputStream? = null

        try {
            val url = URL(release.apkDownloadUrl)
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
            connection = (url.openConnection() as HttpURLConnection).apply {
                connectTimeout = CONNECT_TIMEOUT_MS
                readTimeout = READ_TIMEOUT_MS
                requestMethod = "GET"
                setRequestProperty("Accept", "application/vnd.github+json")
                setRequestProperty("User-Agent", "APEX-Root-Updater")
                instanceFollowRedirects = true
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
