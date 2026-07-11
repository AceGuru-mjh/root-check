package com.apex.root.service

import android.app.Notification
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.IBinder
import android.util.Log
import androidx.core.app.NotificationCompat
import androidx.core.content.ContextCompat
import com.apex.root.MainActivity
import com.apex.root.core.notification.Notifier
import com.apex.root.domain.parallel.RealtimeGuardMonitor
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch

/**
 * 实时监控前台服务 — v1.1.1 新增。
 *
 * 利用 FOREGROUND_SERVICE / FOREGROUND_SERVICE_SPECIAL_USE 权限,
 * 在后台持续运行 RealtimeGuardMonitor,即使应用退到后台也能:
 *  - 周期性检测 root 痕迹变化
 *  - 发现新安装 root 工具时立即推送通知
 *  - 通知栏常驻显示监控状态
 *
 * 生命周期:
 *  - startService(context, intervalMs) → 启动前台服务
 *  - stopService(context) → 停止服务
 *  - 服务内部持有 RealtimeGuardMonitor 实例
 */
class GuardMonitorService : Service() {

    companion object {
        private const val TAG = "GuardMonitorSvc"
        const val NOTIF_ID = 2001
        const val ACTION_START = "com.apex.root.action.START_GUARD"
        const val ACTION_STOP = "com.apex.root.action.STOP_GUARD"
        const val EXTRA_INTERVAL_MS = "interval_ms"
        const val DEFAULT_INTERVAL_MS = 60_000L

        /**
         * 启动实时监控前台服务。
         * Android 12+ 要求前台服务启动时应用在前台,本方法应在 Activity onResume 中调用。
         */
        fun startService(context: Context, intervalMs: Long = DEFAULT_INTERVAL_MS) {
            val intent = Intent(context, GuardMonitorService::class.java).apply {
                action = ACTION_START
                putExtra(EXTRA_INTERVAL_MS, intervalMs)
            }
            try {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    ContextCompat.startForegroundService(context, intent)
                } else {
                    context.startService(intent)
                }
                Log.i(TAG, "Guard monitor service start requested")
            } catch (e: Throwable) {
                Log.e(TAG, "Failed to start guard monitor service", e)
            }
        }

        /**
         * 停止实时监控前台服务。
         */
        fun stopService(context: Context) {
            try {
                context.startService(Intent(context, GuardMonitorService::class.java).apply {
                    action = ACTION_STOP
                })
            } catch (_: Throwable) {}
            try {
                context.stopService(Intent(context, GuardMonitorService::class.java))
            } catch (_: Throwable) {}
            Log.i(TAG, "Guard monitor service stop requested")
        }
    }

    private var guardMonitor: RealtimeGuardMonitor? = null
    private val serviceScope = CoroutineScope(SupervisorJob() + Dispatchers.Default)

    override fun onCreate() {
        super.onCreate()
        // 确保通知通道已创建
        Notifier.createChannels(this)
        // 启动前台通知 (必须在 onStartCommand 之前或之中调用 startForeground)
        startForeground(NOTIF_ID, buildPersistentNotification("实时监控启动中"))
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            ACTION_STOP -> {
                stopGuardMonitor()
                stopForeground(STOP_FOREGROUND_REMOVE)
                stopSelf()
                return START_NOT_STICKY
            }
            ACTION_START -> {
                val interval = intent.getLongExtra(EXTRA_INTERVAL_MS, DEFAULT_INTERVAL_MS)
                startGuardMonitor(interval)
            }
        }
        return START_STICKY  // 服务被杀后自动重启
    }

    private fun startGuardMonitor(intervalMs: Long) {
        if (guardMonitor != null) {
            Log.w(TAG, "Guard monitor already running")
            return
        }
        val monitor = RealtimeGuardMonitor(applicationContext)
        guardMonitor = monitor

        // 订阅告警,更新通知
        serviceScope.launch {
            monitor.alerts.collect { alert ->
                updateNotification(
                    when (alert.level) {
                        RealtimeGuardMonitor.AlertLevel.CRITICAL -> "🚨 ${alert.title}"
                        RealtimeGuardMonitor.AlertLevel.ALERT -> "⚠️ ${alert.title}"
                        RealtimeGuardMonitor.AlertLevel.WARNING -> "⚠️ ${alert.title}"
                        RealtimeGuardMonitor.AlertLevel.INFO -> "ℹ️ ${alert.title}"
                    }
                )
            }
        }
        // 订阅状态,更新通知文本
        serviceScope.launch {
            monitor.state.collect { state ->
                if (state.isActive) {
                    updateNotification("实时监控中 · 间隔 ${state.intervalMs / 1000}s · 异常 ${state.consecutiveAnomalies}")
                }
            }
        }
        monitor.start(intervalMs)
        Log.i(TAG, "Guard monitor started, interval=${intervalMs}ms")
    }

    private fun stopGuardMonitor() {
        guardMonitor?.stop()
        guardMonitor = null
        serviceScope.cancel()
        Log.i(TAG, "Guard monitor stopped")
    }

    private fun buildPersistentNotification(text: String): Notification {
        val pendingIntent = PendingIntent.getActivity(
            this, 0,
            Intent(this, MainActivity::class.java).apply {
                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TOP)
            },
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )
        return NotificationCompat.Builder(this, Notifier.CHANNEL_GUARD)
            .setSmallIcon(android.R.drawable.ic_menu_compass)
            .setContentTitle("APEX Root 实时防护")
            .setContentText(text)
            .setOngoing(true)
            .setContentIntent(pendingIntent)
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .setCategory(NotificationCompat.CATEGORY_SERVICE)
            .build()
    }

    private fun updateNotification(text: String) {
        try {
            val nm = ContextCompat.getSystemService(this, android.app.NotificationManager::class.java)
            nm?.notify(NOTIF_ID, buildPersistentNotification(text))
        } catch (e: Throwable) {
            Log.w(TAG, "Failed to update notification: ${e.message}")
        }
    }

    override fun onDestroy() {
        stopGuardMonitor()
        super.onDestroy()
    }

    override fun onBind(intent: Intent?): IBinder? = null
}
