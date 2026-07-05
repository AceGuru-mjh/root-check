package com.apex.root.core.permission

import android.Manifest
import android.accessibilityservice.AccessibilityServiceInfo
import android.app.AppOpsManager
import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import android.os.Environment
import android.os.Process
import android.provider.Settings
import android.util.Log
import androidx.core.content.ContextCompat
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlinx.coroutines.withTimeoutOrNull

/**
 * 统一权限管理器。
 *
 * 集中检查 / 申请本应用所需的所有权限：
 *  - ROOT（su 二进制可用且已授权）
 *  - 通知权限（Android 13+ 运行时）
 *  - 存储权限（Android 12 及以下 READ_EXTERNAL_STORAGE；Android 11+ MANAGE_EXTERNAL_STORAGE）
 *  - 无障碍服务（需用户在系统设置中手动开启，无法运行时弹窗）
 *  - 悬浮窗 / 显示在其他应用上层（SYSTEM_ALERT_WINDOW）
 *  - 使用情况访问权限（PACKAGE_USAGE_STATS，用于危险应用检测）
 *
 * 参考的开源实现：
 *  - topjohnwu/Magisk 的 root 授权检测（su -c id，带超时）
 *  - RikkaApps/Shizuku / RikkaApps/AppOps 的特殊权限状态查询
 *  - MuntashirAkon/AppManager 的无障碍服务启用检测
 */
object PermissionManager {

    private const val TAG = "ApexPerms"

    /** 权限类型 */
    enum class PermType {
        ROOT,
        NOTIFICATION,
        STORAGE,
        ACCESSIBILITY,
        OVERLAY,
        USAGE_STATS
    }

    /** 权限状态 */
    enum class PermState {
        GRANTED,       // 已授权
        DENIED,        // 未授权（可运行时申请）
        SPECIAL,       // 需跳转系统设置页授权（无法运行时弹窗）
        UNAVAILABLE    // 当前系统版本不适用 / 不可用
    }

    data class PermissionInfo(
        val type: PermType,
        val state: PermState,
        val detail: String
    )

    /**
     * 检测 Root 状态。带 2.5s 超时，避免 su 弹窗阻塞。
     * - 设备无 su 二进制 → UNAVAILABLE
     * - su 存在但未授权 → DENIED
     * - su 已授权（uid=0）→ GRANTED
     *
     * 修复：原实现 process.waitFor() 是阻塞调用，不响应协程取消。当 withTimeoutOrNull
     * 触发取消时，IO 线程仍然阻塞在 waitFor() 上，导致 Dispatchers.IO 线程被耗尽。
     * 现使用 process.waitFor(timeout, TimeUnit.MILLISECONDS) + destroyForcibly() 兜底。
     */
    suspend fun checkRoot(): PermissionInfo = withContext(Dispatchers.IO) {
        var process: java.lang.Process? = null
        try {
            val result = withTimeoutOrNull(2500L) {
                runCatching {
                    process = Runtime.getRuntime().exec(arrayOf("su", "-c", "id"))
                    val p = process!!
                    // 用带超时的 waitFor，避免无限阻塞
                    val text = p.inputStream.bufferedReader().readText()
                    val errText = p.errorStream.bufferedReader().readText()
                    val exited = p.waitFor(2500, java.util.concurrent.TimeUnit.MILLISECONDS)
                    val exit = if (exited) p.exitValue() else -1
                    Triple(exit, text, errText)
                }.getOrNull()
            }

            when {
                result == null -> PermissionInfo(
                    PermType.ROOT, PermState.DENIED,
                    "检测超时（su 未响应，可能需用户在弹窗中授权）"
                )
                result.first == 0 && result.second.contains("uid=0") -> PermissionInfo(
                    PermType.ROOT, PermState.GRANTED,
                    "已获取（uid=0）"
                )
                result.second.isEmpty() && result.third.contains("not found") -> PermissionInfo(
                    PermType.ROOT, PermState.UNAVAILABLE,
                    "未安装 root 框架"
                )
                else -> PermissionInfo(
                    PermType.ROOT, PermState.DENIED,
                    "未授权（exit=${result.first}）"
                )
            }
        } finally {
            // 兜底清理：如果 su 进程仍在运行（用户没响应弹窗），强制销毁避免泄漏
            try {
                process?.destroyForcibly()
            } catch (_: Throwable) {}
        }
    }

    /** 通知权限（Android 13+ 需运行时申请；低版本默认授予） */
    fun checkNotification(context: Context): PermissionInfo {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            val granted = ContextCompat.checkSelfPermission(
                context, Manifest.permission.POST_NOTIFICATIONS
            ) == PackageManager.PERMISSION_GRANTED
            PermissionInfo(
                PermType.NOTIFICATION,
                if (granted) PermState.GRANTED else PermState.DENIED,
                if (granted) "已授权" else "未授权（用于推送扫描结果与告警）"
            )
        } else {
            PermissionInfo(PermType.NOTIFICATION, PermState.GRANTED, "当前版本默认授予")
        }
    }

    /**
     * 存储权限。
     * - Android 13+：应用专属目录无需权限；导出日志到任意目录需 MANAGE_EXTERNAL_STORAGE（特殊权限）
     * - Android 11-12：MANAGE_EXTERNAL_STORAGE（特殊权限）或 READ_EXTERNAL_STORAGE（运行时）
     * - Android 10 及以下：READ/WRITE_EXTERNAL_STORAGE（运行时）
     */
    fun checkStorage(context: Context): PermissionInfo {
        val manageAllGranted = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            Environment.isExternalStorageManager()
        } else false

        if (manageAllGranted) {
            return PermissionInfo(
                PermType.STORAGE, PermState.GRANTED,
                "所有文件访问（可导出日志到任意目录）"
            )
        }

        if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.S) {
            val read = ContextCompat.checkSelfPermission(
                context, Manifest.permission.READ_EXTERNAL_STORAGE
            ) == PackageManager.PERMISSION_GRANTED
            return PermissionInfo(
                PermType.STORAGE,
                if (read) PermState.GRANTED else PermState.DENIED,
                if (read) "存储读取已授权" else "未授权（保存日志需要）"
            )
        }

        return PermissionInfo(
            PermType.STORAGE, PermState.SPECIAL,
            "未授权（需在设置中开启「所有文件访问权限」以导出日志）"
        )
    }

    /**
     * 无障碍服务状态。
     * Android 不允许运行时弹窗申请无障碍权限，必须引导用户到系统设置页开启。
     */
    fun checkAccessibility(context: Context): PermissionInfo {
        return try {
            val enabled = isAccessibilityEnabled(context)
            if (enabled) {
                PermissionInfo(
                    PermType.ACCESSIBILITY, PermState.GRANTED,
                    "已启用（可用于行为监控与实时防护）"
                )
            } else {
                PermissionInfo(
                    PermType.ACCESSIBILITY, PermState.SPECIAL,
                    "未启用（需在设置中开启无障碍服务）"
                )
            }
        } catch (e: Throwable) {
            Log.e(TAG, "checkAccessibility failed", e)
            PermissionInfo(PermType.ACCESSIBILITY, PermState.SPECIAL, "检测失败：${e.message}")
        }
    }

    /** 悬浮窗权限（SYSTEM_ALERT_WINDOW），用于守护进程告警弹窗 */
    fun checkOverlay(context: Context): PermissionInfo {
        val granted = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            Settings.canDrawOverlays(context)
        } else true
        return PermissionInfo(
            PermType.OVERLAY,
            if (granted) PermState.GRANTED else PermState.SPECIAL,
            if (granted) "已授权" else "未授权（实时防护告警弹窗需要）"
        )
    }

    /** 使用情况访问权限（PACKAGE_USAGE_STATS），用于危险应用检测 */
    fun checkUsageStats(context: Context): PermissionInfo {
        return try {
            val appOps = context.getSystemService(Context.APP_OPS_SERVICE) as AppOpsManager
            val mode = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                appOps.unsafeCheckOpNoThrow(
                    AppOpsManager.OPSTR_GET_USAGE_STATS,
                    Process.myUid(),
                    context.packageName
                )
            } else {
                @Suppress("DEPRECATION")
                appOps.checkOpNoThrow(
                    AppOpsManager.OPSTR_GET_USAGE_STATS,
                    Process.myUid(),
                    context.packageName
                )
            }
            val granted = mode == AppOpsManager.MODE_ALLOWED
            PermissionInfo(
                PermType.USAGE_STATS,
                if (granted) PermState.GRANTED else PermState.SPECIAL,
                if (granted) "已授权（可枚举运行中应用）" else "未授权（危险应用检测需要）"
            )
        } catch (e: Throwable) {
            PermissionInfo(PermType.USAGE_STATS, PermState.SPECIAL, "检测失败：${e.message}")
        }
    }

    /**
     * 聚合所有权限状态。Root 检测在 IO 线程执行（带超时），其余同步。
     */
    suspend fun checkAll(context: Context): List<PermissionInfo> {
        val root = checkRoot()
        return listOf(
            root,
            checkNotification(context),
            checkStorage(context),
            checkAccessibility(context),
            checkOverlay(context),
            checkUsageStats(context)
        )
    }

    /** 判断是否有已启用的无障碍服务绑定到本应用 */
    private fun isAccessibilityEnabled(context: Context): Boolean {
        return try {
            val am = context.getSystemService(Context.ACCESSIBILITY_SERVICE)
                as android.view.accessibility.AccessibilityManager
            if (!am.isEnabled) return false
            val enabledServices = am.getEnabledAccessibilityServiceList(
                AccessibilityServiceInfo.FEEDBACK_GENERIC
            )
            val targetService = "${context.packageName}/.service.ApexAccessibilityService"
            val canonicalTarget = android.content.ComponentName.unflattenFromString(targetService)
                ?.flattenToString() ?: targetService
            enabledServices.any { info ->
                val resolved = info.resolveInfo?.serviceInfo
                val flat = if (resolved != null) {
                    android.content.ComponentName(resolved.packageName, resolved.name).flattenToString()
                } else ""
                flat == canonicalTarget || flat == targetService
            }
        } catch (e: Throwable) {
            false
        }
    }
}
