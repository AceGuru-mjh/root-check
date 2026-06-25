package com.rootdetector.core.security

import android.content.Context
import android.content.pm.PackageManager
import android.os.Process
import java.io.File

object SelfProtection {

    private var verified = false

    fun verifyApkIntegrity(context: Context): Boolean {
        try {
            val info = context.packageManager.getPackageInfo(
                context.packageName,
                PackageManager.GET_SIGNATURES
            )
            if (info.signatures.isNotEmpty()) {
                val certHash = sha256(info.signatures[0].toByteArray())
                verified = true
                return true
            }
        } catch (_: Exception) {}
        return false
    }

    fun isDebuggerAttached(): Boolean {
        return Process.myUid() == 0 ||
               android.os.Debug.isDebuggerConnected() ||
               android.os.Debug.waitingForDebugger()
    }

    fun isPtraced(): Boolean {
        return try {
            val tracerPid = File("/proc/self/status")
                .readText()
                .lines()
                .firstOrNull { it.startsWith("TracerPid:") }
                ?.split(":")
                ?.getOrNull(1)
                ?.trim()
                ?.toIntOrNull() ?: 0
            tracerPid != 0
        } catch (_: Exception) {
            false
        }
    }

    fun detectHook(): List<String> {
        val found = mutableListOf<String>()
        try {
            val maps = File("/proc/self/maps").readText()
            val hookLibraries = listOf("frida", "xposed", "substrate", "cydia",
                                       "libinject", "gum-js-loop", "libriru",
                                       "liblspd", "libwhale", "libdobby")
            for (lib in hookLibraries) {
                if (maps.contains(lib)) {
                    found.add(lib)
                }
            }
        } catch (_: Exception) {}
        return found
    }

    private fun sha256(data: ByteArray): String {
        try {
            val md = java.security.MessageDigest.getInstance("SHA-256")
            return md.digest(data).joinToString("") { "%02x".format(it) }
        } catch (_: Exception) {
            return ""
        }
    }
}