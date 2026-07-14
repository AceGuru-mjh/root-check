package com.apex.root.core.notification

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import androidx.core.app.NotificationCompat
import androidx.core.content.ContextCompat
import com.apex.root.ui.MainActivity

/**
 * 通知推送管理器。
 *
 * 借鉴：
 *  - topjohnwu/Magisk 的 NotificationMgr：单一 channel 创建点 + 静态工厂方法
 *  - termux/termux-app 的 TermuxNotification：每个功能一个 channel，区分重要级别
 *
 * 5 个通知通道对应 5 个 notify* 设置：
 *  - apex_scan_complete (扫描完成) — IMPORTANCE_LOW
 *  - apex_risk_alert (风险告警) — IMPORTANCE_HIGH
 *  - apex_guard (实时防护) — IMPORTANCE_DEFAULT
 *  - apex_cure (治愈结果) — IMPORTANCE_LOW
 *  - apex_update (更新可用) — IMPORTANCE_DEFAULT
 */
object Notifier {

    const val CHANNEL_SCAN_COMPLETE = "apex_scan_complete"
    const val CHANNEL_RISK_ALERT = "apex_risk_alert"
    const val CHANNEL_GUARD = "apex_guard"
    const val CHANNEL_CURE = "apex_cure"
    const val CHANNEL_UPDATE = "apex_update"

    const val NOTIF_ID_SCAN_COMPLETE = 1001
    const val NOTIF_ID_RISK_ALERT = 1002
    const val NOTIF_ID_GUARD = 1003
    const val NOTIF_ID_CURE = 1004
    const val NOTIF_ID_UPDATE = 1005

    fun createChannels(context: Context) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return
        val nm = ContextCompat.getSystemService(context, NotificationManager::class.java) ?: return

        val channels = listOf(
            NotificationChannel(CHANNEL_SCAN_COMPLETE, "扫描完成", NotificationManager.IMPORTANCE_LOW).apply {
                description = "扫描完成时通知"; setShowBadge(false)
            },
            NotificationChannel(CHANNEL_RISK_ALERT, "风险告警", NotificationManager.IMPORTANCE_HIGH).apply {
                description = "检测到高风险时通知"; enableVibration(true); enableLights(true)
            },
            NotificationChannel(CHANNEL_GUARD, "实时防护", NotificationManager.IMPORTANCE_DEFAULT).apply {
                description = "Guard 实时防护告警"; setShowBadge(true)
            },
            NotificationChannel(CHANNEL_CURE, "治愈结果", NotificationManager.IMPORTANCE_LOW).apply {
                description = "治愈操作完成时通知"; setShowBadge(false)
            },
            NotificationChannel(CHANNEL_UPDATE, "更新可用", NotificationManager.IMPORTANCE_DEFAULT).apply {
                description = "有新版本可用时通知"; setShowBadge(true)
            }
        )
        channels.forEach { nm.createNotificationChannel(it) }
    }

    private fun buildMainPendingIntent(context: Context): PendingIntent {
        val intent = Intent(context, MainActivity::class.java).apply {
            addFlags(Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TOP)
        }
        val flags = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        } else PendingIntent.FLAG_UPDATE_CURRENT
        return PendingIntent.getActivity(context, 0, intent, flags)
    }

    private fun buildBase(
        context: Context,
        channelId: String,
        title: String,
        text: String,
        smallIcon: Int = android.R.drawable.ic_dialog_info
    ): NotificationCompat.Builder {
        return NotificationCompat.Builder(context, channelId)
            .setSmallIcon(smallIcon)
            .setContentTitle(title)
            .setContentText(text)
            .setContentIntent(buildMainPendingIntent(context))
            .setAutoCancel(true)
            .setStyle(NotificationCompat.BigTextStyle().bigText(text))
    }

    fun notifyScanComplete(context: Context, riskScore: Int, riskCount: Int) {
        try {
            val riskLevel = when {
                riskScore >= 61 -> "高风险"
                riskScore >= 31 -> "中等风险"
                riskScore >= 11 -> "轻度风险"
                else -> "安全"
            }
            val text = "风险分: $riskScore/100 · $riskLevel · 检测到 $riskCount 个异常"
            val notif = buildBase(context, CHANNEL_SCAN_COMPLETE, "扫描完成", text).build()
            notify(context, NOTIF_ID_SCAN_COMPLETE, notif)
        } catch (e: Throwable) {
            android.util.Log.e("Notifier", "notifyScanComplete failed", e)
        }
    }

    fun notifyRiskAlert(context: Context, title: String, detail: String) {
        try {
            val notif = buildBase(context, CHANNEL_RISK_ALERT, "⚠️ $title", detail)
                .setPriority(NotificationCompat.PRIORITY_HIGH)
                .setVibrate(longArrayOf(0, 300, 200, 300))
                .build()
            notify(context, NOTIF_ID_RISK_ALERT, notif)
        } catch (e: Throwable) {
            android.util.Log.e("Notifier", "notifyRiskAlert failed", e)
        }
    }

    fun notifyGuardAlert(context: Context, alert: String) {
        try {
            val notif = buildBase(context, CHANNEL_GUARD, "🛡️ 实时防护告警", alert).build()
            notify(context, NOTIF_ID_GUARD, notif)
        } catch (e: Throwable) {
            android.util.Log.e("Notifier", "notifyGuardAlert failed", e)
        }
    }

    fun notifyCureResult(context: Context, level: String, success: Boolean) {
        try {
            val title = if (success) "✅ $level 完成" else "❌ $level 失败"
            val text = if (success) "治愈操作已成功完成" else "请查看应用内日志了解失败原因"
            val notif = buildBase(context, CHANNEL_CURE, title, text).build()
            notify(context, NOTIF_ID_CURE, notif)
        } catch (e: Throwable) {
            android.util.Log.e("Notifier", "notifyCureResult failed", e)
        }
    }

    fun notifyUpdateAvailable(context: Context, newVersion: String) {
        try {
            val notif = buildBase(
                context, CHANNEL_UPDATE,
                "🔄 发现新版本 v$newVersion",
                "点击查看更新详情并下载"
            ).build()
            notify(context, NOTIF_ID_UPDATE, notif)
        } catch (e: Throwable) {
            android.util.Log.e("Notifier", "notifyUpdateAvailable failed", e)
        }
    }

    fun cancel(context: Context, notifId: Int) {
        try {
            val nm = ContextCompat.getSystemService(context, NotificationManager::class.java) ?: return
            nm.cancel(notifId)
        } catch (e: Throwable) {
            android.util.Log.e("Notifier", "cancel failed", e)
        }
    }

    private fun notify(context: Context, id: Int, notif: android.app.Notification) {
        // 修复 v1.0.7: Android 13+ 必须检查 POST_NOTIFICATIONS 权限,
        // 否则 NotificationManager.notify() 会抛 SecurityException 导致闪退。
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            val granted = ContextCompat.checkSelfPermission(
                context,
                android.Manifest.permission.POST_NOTIFICATIONS
            ) == PackageManager.PERMISSION_GRANTED
            if (!granted) {
                android.util.Log.w("Notifier", "POST_NOTIFICATIONS not granted — skipping notification $id")
                return
            }
        }
        val nm = ContextCompat.getSystemService(context, NotificationManager::class.java) ?: return
        try {
            nm.notify(id, notif)
        } catch (e: SecurityException) {
            android.util.Log.e("Notifier", "notify($id) denied: ${e.message}")
        } catch (e: Throwable) {
            android.util.Log.e("Notifier", "notify($id) failed: ${e.message}")
        }
    }
}
