package com.apex.root

import android.app.Application
import android.util.Log

/**
 * 不要在此处启动前台服务！
 * Android 12+ 要求应用处于前台状态才能启动前台服务，
 * Application.onCreate() 不满足此条件，会导致 ForegroundServiceStartNotAllowedException 崩溃。
 * 前台服务已移至 MainActivity.onResume() 中启动。
 */
class ApexRootApp : Application() {
    override fun onCreate() {
        super.onCreate()
        Thread.setDefaultUncaughtExceptionHandler { thread, throwable ->
            Log.e("ApexRoot", "Uncaught exception on ${thread.name}", throwable)
        }
    }
}
