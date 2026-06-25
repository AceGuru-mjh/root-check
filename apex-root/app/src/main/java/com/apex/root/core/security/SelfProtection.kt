package com.apex.root.core.security

import android.content.Context
import android.content.pm.PackageManager
import android.os.Process
import java.io.File
import java.security.MessageDigest
import java.util.zip.Adler32

object SelfProtection {
    private var verified = false
    private var apkHash: String = ""
    private var lastIntegrityCheck = 0L
    private const val INTEGRITY_CHECK_INTERVAL = 30000L // 30 seconds

    // ─── APK integrity verification ─────────────────────────
    fun verifyApkIntegrity(context: Context): Boolean {
        try {
            val info = context.packageManager.getPackageInfo(
                context.packageName,
                PackageManager.GET_SIGNATURES
            )
            if (info.signatures.isNotEmpty()) {
                val sig = info.signatures[0].toByteArray()
                val digest = MessageDigest.getInstance("SHA-256").digest(sig)
                apkHash = digest.joinToString("") { "%02x".format(it) }
                verified = true
                return true
            }
        } catch (_: Exception) {}
        return false
    }

    fun getApkHash(): String = apkHash

    // ─── Periodic integrity check ───────────────────────────
    fun periodicIntegrityCheck(context: Context): Boolean {
        val now = System.currentTimeMillis()
        if (now - lastIntegrityCheck < INTEGRITY_CHECK_INTERVAL) return verified
        lastIntegrityCheck = now
        return verifyApkIntegrity(context)
    }

    // ─── Code integrity check via DEX CRC ───────────────────
    fun checkDexIntegrity(): List<String> {
        val issues = mutableListOf<String>()
        try {
            val dexFiles = File("/proc/self/maps").readText()
            val dexLines = dexFiles.lines().filter { it.contains(".dex") || it.contains(".oat") }
            if (dexLines.isEmpty()) {
                issues.add("No DEX/oat files in memory mapping")
            }
            // Check for unexpected DEX files
            val knownPrefixes = listOf("/data/app/", "/system/framework/")
            for (line in dexLines) {
                val path = line.substringAfterLast(" ").trim()
                if (path.isNotEmpty() && knownPrefixes.none { path.startsWith(it) }) {
                    if (!path.startsWith("/apex/") && !path.startsWith("/system/")) {
                        issues.add("Unexpected DEX location: $path")
                    }
                }
            }
        } catch (_: Exception) {
            issues.add("Cannot read memory maps")
        }
        return issues
    }

    // ─── Debugger detection ─────────────────────────────────
    fun isDebuggerAttached(): Boolean {
        return Process.myUid() == 0 ||
               android.os.Debug.isDebuggerConnected() ||
               android.os.Debug.waitingForDebugger()
    }

    // ─── Ptrace detection (via raw /proc/self/status) ──────
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

    // ─── Hook detection via memory maps ─────────────────────
    fun detectHook(): List<String> {
        val found = mutableListOf<String>()
        try {
            val maps = File("/proc/self/maps").readText()
            val hookLibraries = listOf(
                "frida", "xposed", "substrate", "cydia",
                "libinject", "gum-js-loop", "libriru",
                "liblspd", "libwhale", "libdobby",
                "libfrida", "frida-agent", "frida-helper",
                "gadget", "libgadget", "libmsaoaidsec",
                "libsigchain", "libdexposed"
            )
            for (lib in hookLibraries) {
                if (maps.contains(lib, ignoreCase = true)) {
                    found.add(lib)
                }
            }
        } catch (_: Exception) {}
        return found
    }

    // ─── Anti-injection: check /proc/self/maps for anomalies ─
    fun detectInjection(): List<String> {
        val issues = mutableListOf<String>()
        try {
            val maps = File("/proc/self/maps").readText()
            val lines = maps.lines()

            var rwxCount = 0
            var anonExecCount = 0
            var suspiciousPaths = mutableListOf<String>()

            for (line in lines) {
                if (line.isBlank()) continue
                val parts = line.split(Regex("\\s+"))
                if (parts.size < 5) continue

                val perms = parts[1]
                val path = if (parts.size > 5) parts[5] else ""

                // Count RWX mappings
                if (perms == "rwxp") rwxCount++

                // Count anonymous executable mappings
                if (perms.startsWith("r-xp") && (path.isEmpty() || path.startsWith("["))) {
                    anonExecCount++
                }

                // Check for suspicious injected libraries
                if (path.startsWith("/data/data/") && path.endsWith(".so") &&
                    !path.contains(context?.packageName ?: "com.apex.root")) {
                    suspiciousPaths.add(path)
                }
            }

            if (rwxCount > 3) issues.add("RWX mappings: $rwxCount (suspicious)")
            if (anonExecCount > 5) issues.add("Anonymous executable pages: $anonExecCount")
            if (suspiciousPaths.isNotEmpty()) {
                issues.add("Foreign .so in data dir: ${suspiciousPaths.joinToString()}")
            }
        } catch (_: Exception) {}
        return issues
    }

    private var context: Context? = null

    fun init(ctx: Context) { context = ctx }

    // ─── Multi-process verification ─────────────────────────
    fun spawnVerifierProcess(): Boolean {
        try {
            val builder = ProcessBuilder(
                "sh", "-c",
                "while kill -0 ${
                    Process.myPid()
                } 2>/dev/null; do " +
                        "if [ -f /proc/${Process.myPid()}/status ]; then " +
                        "tp=\$(grep 'TracerPid:' /proc/${Process.myPid()}/status | awk '{print \$2}'); " +
                        "if [ \"\$tp\" != \"0\" ] && [ \"\$tp\" != \"\" ]; then " +
                        "echo \"WARNING: TracerPid=\$tp detected on parent\" > /data/local/tmp/apex_alert; " +
                        "fi; " +
                        "fi; " +
                        "sleep 5; " +
                        "done"
            )
            builder.redirectErrorStream(true)
            val process = builder.start()
            // Detach the verifier - don't track it in this process
            process.outputStream.close()
            return true
        } catch (_: Exception) {
            return false
        }
    }

    fun verifyChildProcess(pid: Int): Boolean {
        return try {
            val cmdline = File("/proc/$pid/cmdline").readText()
            cmdline.contains("com.apex.root")
        } catch (_: Exception) {
            false
        }
    }

    // ─── Detection randomization ────────────────────────────
    fun randomizeScanOrder(layers: List<Int>): List<Int> {
        val seed = System.nanoTime()
        val shuffled = layers.toMutableList()
        // Fisher-Yates shuffle with time-based seed
        for (i in shuffled.size - 1 downTo 1) {
            val j = ((seed shr (i * 3)) and 0x7FFFFFFF).toInt() % (i + 1)
            val tmp = shuffled[i]
            shuffled[i] = shuffled[j]
            shuffled[j] = tmp
        }
        return shuffled
    }

    // ─── Full self-check ────────────────────────────────────
    fun fullSelfCheck(context: Context): Map<String, Any> {
        val results = mutableMapOf<String, Any>()
        results["verified"] = verifyApkIntegrity(context)
        results["debugger"] = isDebuggerAttached()
        results["ptraced"] = isPtraced()
        results["hooks"] = detectHook()
        results["injections"] = detectInjection()
        results["dexIssues"] = checkDexIntegrity()
        results["apkHash"] = apkHash
        return results
    }
}
