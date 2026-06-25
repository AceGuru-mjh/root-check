package com.apex.root

import android.app.Application
import android.content.Intent
import com.apex.root.core.TrustedDaemonService

class ApexRootApp : Application() {
    override fun onCreate() {
        super.onCreate()
        // Start the trusted daemon service
        startService(Intent(this, TrustedDaemonService::class.java))
    }
}
