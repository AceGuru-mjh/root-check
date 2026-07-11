package com.apex.root.domain.parallel

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import android.os.Environment
import android.provider.MediaStore
import android.util.Log
import androidx.core.content.ContextCompat

/**
 * 媒体扫描检测器 — v1.1.1 新增。
 *
 * P1-9: 利用 READ_MEDIA_IMAGES / READ_MEDIA_VIDEO / READ_MEDIA_AUDIO 权限,
 * 扫描设备上的媒体文件,检测可疑的 root / hack 工具相关文件:
 *  - 下载目录中的 Magisk / KernelSU / APatch APK
 *  - 图片中的 root 工具截图 (文件名特征)
 *  - 视频中的教程文件
 *  - 音频中的录音证据
 *
 * 这是对文件系统检测层 (L1-L20) 的补充,因为 MediaStore 查询不依赖
 * /data/adb 等特权路径,普通应用也能访问 (需 READ_MEDIA_* 权限)。
 */
object MediaScanner {

    private const val TAG = "MediaScanner"

    /** 媒体检测发现的项 */
    data class MediaFinding(
        val fileName: String,
        val filePath: String,
        val mimeType: String,
        val sizeBytes: Long,
        val reason: String   // 为什么被认为是可疑的
    )

    /** 已知的 root / hack 工具文件名特征 */
    private val SUSPICIOUS_PATTERNS = listOf(
        // Root 框架 APK
        "magisk" to "Magisk 框架相关文件",
        "kernelsu" to "KernelSU 框架相关文件",
        "ksu" to "KernelSU 框架相关文件",
        "apatch" to "APatch 框架相关文件",
        "sukisu" to "SukiSU 框架相关文件",
        "supersu" to "SuperSU 框架相关文件",
        "superuser" to "SuperUser 相关文件",
        // Hook / 注入工具
        "frida" to "Frida 注入工具相关文件",
        "xposed" to "Xposed 框架相关文件",
        "lsposed" to "LSPosed 框架相关文件",
        "lspatch" to "LSPatch 工具相关文件",
        "edxposed" to "EdXposed 框架相关文件",
        "substrate" to "Substrate 注入工具相关文件",
        // 内存修改器
        "gameguardian" to "GameGuardian 内存修改器",
        "cheatengine" to "CheatEngine 内存修改器",
        "luckypatcher" to "Lucky Patcher 破解工具",
        "gamekiller" to "GameKiller 内存修改器",
        "xmodgame" to "Xmodgames 游戏修改器",
        // 双开 / 虚拟
        "virtualxposed" to "VirtualXposed 虚拟框架",
        "parallel" to "平行空间双开应用",
        "dualspace" to "双开大师",
        // 其他
        "root" to "Root 相关文件",
        "busybox" to "BusyBox 工具",
        "superuser" to "SuperUser 相关"
    )

    /**
     * 扫描媒体文件。需要 READ_MEDIA_* 权限 (Android 13+) 或
     * READ_EXTERNAL_STORAGE (Android 12 及以下)。
     *
     * @param context Context
     * @return 发现的可疑媒体文件列表 (最多 50 条)
     */
    fun scan(context: Context): List<MediaFinding> {
        val findings = mutableListOf<MediaFinding>()

        // 检查权限
        if (!hasMediaPermission(context)) {
            Log.w(TAG, "No media permission granted, skipping scan")
            return emptyList()
        }

        // 扫描 Downloads 目录 (通过 MediaStore)
        findings.addAll(scanMediaStore(context, MediaStore.Downloads.EXTERNAL_CONTENT_URI, "downloads"))
        // 扫描图片
        findings.addAll(scanMediaStore(context, MediaStore.Images.Media.EXTERNAL_CONTENT_URI, "image"))
        // 扫描视频
        findings.addAll(scanMediaStore(context, MediaStore.Video.Media.EXTERNAL_CONTENT_URI, "video"))
        // 扫描音频
        findings.addAll(scanMediaStore(context, MediaStore.Audio.Media.EXTERNAL_CONTENT_URI, "audio"))

        Log.i(TAG, "Media scan complete: ${findings.size} suspicious files found")
        return findings.take(50)  // 限制返回数量
    }

    /**
     * 检查是否有媒体访问权限。
     */
    fun hasMediaPermission(context: Context): Boolean {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            // Android 13+: READ_MEDIA_IMAGES / VIDEO / AUDIO
            ContextCompat.checkSelfPermission(context, Manifest.permission.READ_MEDIA_IMAGES) ==
                PackageManager.PERMISSION_GRANTED ||
            ContextCompat.checkSelfPermission(context, Manifest.permission.READ_MEDIA_VIDEO) ==
                PackageManager.PERMISSION_GRANTED ||
            ContextCompat.checkSelfPermission(context, Manifest.permission.READ_MEDIA_AUDIO) ==
                PackageManager.PERMISSION_GRANTED ||
            ContextCompat.checkSelfPermission(context, Manifest.permission.READ_EXTERNAL_STORAGE) ==
                PackageManager.PERMISSION_GRANTED
        } else {
            ContextCompat.checkSelfPermission(context, Manifest.permission.READ_EXTERNAL_STORAGE) ==
                PackageManager.PERMISSION_GRANTED
        }
    }

    /**
     * 扫描指定 MediaStore Uri,查找文件名匹配可疑模式的条目。
     */
    private fun scanMediaStore(
        context: Context,
        uri: android.net.Uri,
        type: String
    ): List<MediaFinding> {
        val findings = mutableListOf<MediaFinding>()
        try {
            val projection = arrayOf(
                MediaStore.MediaColumns.DISPLAY_NAME,
                MediaStore.MediaColumns.DATA,
                MediaStore.MediaColumns.MIME_TYPE,
                MediaStore.MediaColumns.SIZE
            )
            // 只查最近 1000 条,避免全表扫描
            val sortOrder = "${MediaStore.MediaColumns.DATE_ADDED} DESC LIMIT 1000"

            context.contentResolver.query(uri, projection, null, null, sortOrder)?.use { cursor ->
                val nameIdx = cursor.getColumnIndexOrThrow(MediaStore.MediaColumns.DISPLAY_NAME)
                val pathIdx = cursor.getColumnIndexOrThrow(MediaStore.MediaColumns.DATA)
                val mimeIdx = cursor.getColumnIndexOrThrow(MediaStore.MediaColumns.MIME_TYPE)
                val sizeIdx = cursor.getColumnIndexOrThrow(MediaStore.MediaColumns.SIZE)

                while (cursor.moveToNext()) {
                    val name = cursor.getString(nameIdx) ?: continue
                    val path = cursor.getString(pathIdx) ?: ""
                    val mime = cursor.getString(mimeIdx) ?: ""
                    val size = cursor.getLong(sizeIdx)

                    val lowerName = name.lowercase()
                    for ((pattern, reason) in SUSPICIOUS_PATTERNS) {
                        if (lowerName.contains(pattern)) {
                            findings.add(MediaFinding(name, path, mime, size, reason))
                            break  // 一个文件只匹配一个原因
                        }
                    }
                }
            }
        } catch (e: Throwable) {
            Log.w(TAG, "Failed to scan $type: ${e.message}")
        }
        return findings
    }

    /**
     * 生成人类可读的扫描报告。
     */
    fun generateReport(findings: List<MediaFinding>): String {
        if (findings.isEmpty()) return "媒体扫描: 未发现可疑文件"
        val sb = StringBuilder()
        sb.appendLine("媒体扫描: 发现 ${findings.size} 个可疑文件")
        sb.appendLine()
        findings.take(20).forEach { f ->
            sb.appendLine("- ${f.fileName}")
            sb.appendLine("  路径: ${f.filePath}")
            sb.appendLine("  原因: ${f.reason}")
            sb.appendLine("  大小: ${f.sizeBytes} bytes")
            sb.appendLine()
        }
        if (findings.size > 20) {
            sb.appendLine("... 还有 ${findings.size - 20} 个文件未显示")
        }
        return sb.toString()
    }
}
