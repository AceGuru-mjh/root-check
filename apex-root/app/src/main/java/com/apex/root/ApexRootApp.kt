package com.apex.root

import android.app.Application
import android.os.Process
import android.util.Log
import com.apex.root.core.notification.Notifier
import com.apex.root.data.SettingsRepository
import com.apex.root.work.UpdateCheckWorker
import java.io.File
import java.io.FileWriter
import java.io.PrintWriter
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * 不要在此处启动前台服务！
 * Android 12+ 要求应用处于前台状态才能启动前台服务，
 * Application.onCreate() 不满足此条件，会导致 ForegroundServiceStartNotAllowedException 崩溃。
 * 前台服务已移至 MainActivity.onResume() 中启动。
 */
class ApexRootApp : Application() {

    companion object {
        private const val TAG = "ApexRoot"
        private const val CRASH_DIR = "crashes"
        private const val MAX_CRASH_FILES = 5
    }

    override fun onCreate() {
        super.onCreate()
        installCrashHandler()
        // 创建通知通道（O+ 要求在 notify 前创建）
        Notifier.createChannels(this)
        // 启动时根据设置调度周期性更新检查（幂等）
        scheduleUpdateCheckIfEnabled()
    }

    /**
     * 读取 autoCheckUpdates 设置，若开启则调度周期性更新检查 Worker。
     * 使用 ExistingPeriodicWorkPolicy.KEEP 保证幂等性。
     */
    private fun scheduleUpdateCheckIfEnabled() {
        try {
            val settings = SettingsRepository(this).load()
            if (settings.autoCheckUpdates) {
                UpdateCheckWorker.schedule(this)
            }
        } catch (e: Throwable) {
            Log.e(TAG, "Failed to schedule update check on startup", e)
        }
    }

    private fun installCrashHandler() {
        val defaultHandler = Thread.getDefaultUncaughtExceptionHandler()
        Thread.setDefaultUncaughtExceptionHandler { thread, throwable ->
            Log.e(TAG, "Uncaught exception on ${thread.name}", throwable)
            saveCrashReport(thread, throwable)
            defaultHandler?.uncaughtException(thread, throwable)
        }
    }

    /**
     * Write crash report to local file for later diagnosis.
     * Keeps at most [MAX_CRASH_FILES] reports to avoid disk usage.
     */
    private fun saveCrashReport(thread: Thread, throwable: Throwable) {
        try {
            val dir = File(filesDir, CRASH_DIR)
            if (!dir.exists()) dir.mkdirs()

            // Clean old reports
            val files = dir.listFiles()?.sortedByDescending { it.lastModified() }
            files?.drop(MAX_CRASH_FILES - 1)?.forEach { it.delete() }

            val timestamp = SimpleDateFormat("yyyyMMdd_HHmmss_SSS", Locale.US).format(Date())
            val file = File(dir, "crash_$timestamp.log")

            FileWriter(file).use { writer ->
                PrintWriter(writer).use { pw ->
                    pw.println("=== APEX Root Crash Report ===")
                    pw.println("Time: ${Date()}")
                    pw.println("Thread: ${thread.name} (id=${thread.id})")
                    pw.println("PID: ${Process.myPid()}")
                    pw.println("Android: ${android.os.Build.VERSION.RELEASE} (API ${android.os.Build.VERSION.SDK_INT})")
                    pw.println("Device: ${android.os.Build.MANUFACTURER} ${android.os.Build.MODEL}")
                    pw.println()
                    pw.println("--- Stack Trace ---")
                    throwable.printStackTrace(pw)

                    val cause = throwable.cause
                    if (cause != null && cause !== throwable) {
                        pw.println()
                        pw.println("--- Caused by ---")
                        cause.printStackTrace(pw)
                    }
                }
            }
            Log.d(TAG, "Crash report saved to ${file.absolutePath}")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to save crash report", e)
        }
    }
}