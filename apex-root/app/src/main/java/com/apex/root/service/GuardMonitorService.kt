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
import com.apex.root.ui.MainActivity
import com.apex.root.core.notification.Notifier
import com.apex.root.domain.parallel.RealtimeGuardMonitor
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
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
            } catch (e: Throwable) { Log.w(TAG, "caught: ${e.message}", e) }
            try {
                context.stopService(Intent(context, GuardMonitorService::class.java))
            } catch (e: Throwable) { Log.w(TAG, "caught: ${e.message}", e) }
            Log.i(TAG, "Guard monitor service stop requested")
        }
    }

    private var guardMonitor: RealtimeGuardMonitor? = null
    // P0-K3/K4 修复 (v1.1.1): serviceScope 改为 var, 允许 stopGuardMonitor 后重建,
    // 避免在 START_STICKY 重启场景下 scope 已 cancel 导致启动后立即被取消。
    // 同时 stopGuardMonitor 不再在主线程 runBlocking join, 避免 ANR。
    private var serviceScope: CoroutineScope = CoroutineScope(SupervisorJob() + Dispatchers.Default)
        get() {
            // 若 scope 已被 cancel (例如上一次 stopGuardMonitor 后), 重新构造一个。
            // 这不仅修复了 P0-K4 的重启问题, 也保证 onStartCommand 在 START_STICKY
            // 重启场景下能正确启动新协程。
            if (!field.isActive) {
                field = CoroutineScope(SupervisorJob() + Dispatchers.Default)
            }
            return field
        }

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

    // P0-K3 修复 (v1.1.1): 不再在主线程 runBlocking join, 避免 ANR。
    // scope.cancel() 会取消所有子协程, 让它们异步退出; 调用方不需同步等待。
    // 若需资源真正释放, 后续走 onDestroy() 后由系统回收 (Service 进程退出时)。
    private fun stopGuardMonitor() {
        guardMonitor?.stop()
        guardMonitor = null
        // 取消所有子协程 (异步退出, 不阻塞主线程)
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
        // v1.1.3 P2-K4: 加 stop action 让用户快速停止监控
        // 原通知只设 setContentIntent (点击跳主界面), 用户想停止监控需进入
        // 应用 → 设置 → 关闭开关, 操作路径过长。现增加"停止"动作按钮,
        // 点击即发送 ACTION_STOP 到本服务, onStartCommand 收到后走 stopGuardMonitor。
        // 用 getService 而非 getBroadcast: ACTION_STOP 由 onStartCommand 处理,
        // 复用 Service 生命周期管理, 不需新增 BroadcastReceiver。
        val stopIntent = Intent(this, GuardMonitorService::class.java).apply {
            action = ACTION_STOP
        }
        val stopPendingIntent = PendingIntent.getService(
            this, 1, stopIntent,
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )
        return NotificationCompat.Builder(this, Notifier.CHANNEL_GUARD)
            .setSmallIcon(android.R.drawable.ic_menu_compass)
            .setContentTitle("APEX Root 实时防护")
            .setContentText(text)
            .setOngoing(true)
            .setContentIntent(pendingIntent)
            .addAction(android.R.drawable.ic_media_pause, "停止", stopPendingIntent)
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
