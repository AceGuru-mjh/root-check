package com.apex.root.work

import android.content.Context
import android.util.Log
import androidx.work.Constraints
import androidx.work.CoroutineWorker
import androidx.work.ExistingPeriodicWorkPolicy
import androidx.work.NetworkType
import androidx.work.PeriodicWorkRequestBuilder
import androidx.work.WorkManager
import androidx.work.WorkerParameters
import com.apex.root.core.notification.Notifier
import com.apex.root.data.SettingsRepository
import com.apex.root.data.UpdateChannel
import com.apex.root.data.updater.AppUpdater
import com.apex.root.data.updater.UpdateChannelPreference
import com.apex.root.data.updater.UpdateCheckResult
import java.util.concurrent.TimeUnit

/**
 * 周期性更新检查 Worker。
 *
 * 借鉴：
 *  - topjohnwu/Magisk 的 UpdateCheckService：用 WorkManager 周期性检查 GitHub releases
 *  - Android 官方 WorkManager codelab：Constraints + ExistingPeriodicWorkPolicy.KEEP
 *
 * 调度策略：
 *  - 周期：每 24 小时一次
 *  - 约束：仅在网络可用时执行
 *  - 唯一性：使用 enqueueUniquePeriodicWork + KEEP，避免重复调度
 *
 * 触发时机：
 *  - 用户在设置中开启 autoCheckUpdates 时调度
 *  - 用户关闭时取消
 *  - App 启动时若设置已开启但 Worker 未调度，自动补调度
 */
class UpdateCheckWorker(
    appContext: Context,
    params: WorkerParameters
) : CoroutineWorker(appContext, params) {

    companion object {
        const val TAG = "UpdateCheckWorker"
        const val UNIQUE_WORK_NAME = "apex_update_check_periodic"

        /**
         * 调度周期性更新检查。
         * 应在用户开启 autoCheckUpdates 时调用，以及 App 启动时调用（幂等）。
         */
        fun schedule(context: Context) {
            try {
                val constraints = Constraints.Builder()
                    .setRequiredNetworkType(NetworkType.CONNECTED)
                    .build()

                val request = PeriodicWorkRequestBuilder<UpdateCheckWorker>(
                    24, TimeUnit.HOURS
                )
                    .setConstraints(constraints)
                    .addTag(TAG)
                    .build()

                WorkManager.getInstance(context).enqueueUniquePeriodicWork(
                    UNIQUE_WORK_NAME,
                    ExistingPeriodicWorkPolicy.KEEP,
                    request
                )
                Log.i(TAG, "Periodic update check scheduled (every 24h)")
            } catch (e: Throwable) {
                Log.e(TAG, "Failed to schedule periodic update check", e)
            }
        }

        /**
         * 取消周期性更新检查。
         * 应在用户关闭 autoCheckUpdates 时调用。
         */
        fun cancel(context: Context) {
            try {
                WorkManager.getInstance(context).cancelUniqueWork(UNIQUE_WORK_NAME)
                Log.i(TAG, "Periodic update check cancelled")
            } catch (e: Throwable) {
                Log.e(TAG, "Failed to cancel periodic update check", e)
            }
        }

        /**
         * 立即触发一次更新检查（一次性 Worker）。
         * 用于用户手动点击「检查更新」时的后台执行（虽然 UI 通常直接调用 AppUpdater）。
         */
        fun runOnce(context: Context) {
            try {
                val constraints = Constraints.Builder()
                    .setRequiredNetworkType(NetworkType.CONNECTED)
                    .build()
                val request = androidx.work.OneTimeWorkRequestBuilder<UpdateCheckWorker>()
                    .setConstraints(constraints)
                    .addTag(TAG)
                    .build()
                WorkManager.getInstance(context).enqueue(request)
            } catch (e: Throwable) {
                Log.e(TAG, "Failed to enqueue one-time update check", e)
            }
        }
    }

    override suspend fun doWork(): Result {
        return try {
            val context = applicationContext
            val settings = SettingsRepository(context).load()

            // 如果用户已关闭自动检查，直接返回成功（不再继续）
            if (!settings.autoCheckUpdates) {
                Log.i(TAG, "autoCheckUpdates is off, skipping")
                return Result.success()
            }

            val channel = UpdateChannelPreference.fromValue(settings.updateChannel.value)
            val updater = AppUpdater.getInstance(context)

            val result = updater.checkForUpdates(channel, wifiOnly = settings.wifiOnlyUpdate)

            when (result) {
                is UpdateCheckResult.UpToDate -> {
                    Log.i(TAG, "App is up to date")
                }
                is UpdateCheckResult.Available -> {
                    Log.i(TAG, "Update available: ${result.newVersion}")
                    // 仅在用户开启「更新可用通知」时推送
                    if (settings.notifyUpdateAvailable) {
                        Notifier.notifyUpdateAvailable(context, result.newVersion)
                    }
                }
                is UpdateCheckResult.NetworkNotSatisfied -> {
                    Log.i(TAG, "Network not satisfied (Wi-Fi required), skipping")
                }
                is UpdateCheckResult.Error -> {
                    Log.w(TAG, "Update check failed: ${result.message}")
                    // 失败不重试（下次周期再试），避免耗电
                }
            }

            Result.success()
        } catch (e: Throwable) {
            Log.e(TAG, "Update check worker failed", e)
            Result.failure()
        }
    }
}
