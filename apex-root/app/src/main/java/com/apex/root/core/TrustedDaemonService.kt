package com.apex.root.core

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Intent
import android.os.IBinder

class TrustedDaemonService : Service() {
    override fun onCreate() {
        super.onCreate()
        val channelId = "apex_root_daemon"
        val channel = NotificationChannel(
            channelId, "APEX Root Daemon",
            NotificationManager.IMPORTANCE_LOW
        )
        val nm = getSystemService(NotificationManager::class.java)
        nm.createNotificationChannel(channel)

        val notification = Notification.Builder(this, channelId)
            .setContentTitle("APEX Root")
            .setContentText("Trusted daemon running")
            .setSmallIcon(android.R.drawable.ic_menu_manage)
            .setOngoing(true)
            .build()
        startForeground(1, notification)

        loadNativeLibrary()
    }

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        return START_STICKY
    }

    private fun loadNativeLibrary() {
        try {
            System.loadLibrary("apex_root")
        } catch (e: UnsatisfiedLinkError) {
            // Library will be loaded by individual native bridge classes
        }
    }

    companion object {
        init {
            try {
                System.loadLibrary("apex_root")
            } catch (_: UnsatisfiedLinkError) {}
        }
    }
}
