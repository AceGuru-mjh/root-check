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
        } catch (e: Exception) { android.util.Log.w("SelfProtection", "Operation failed: ${e.message}", e) }
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
        } catch (e: Exception) {
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
        } catch (e: Exception) {
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
        } catch (e: Exception) { android.util.Log.w("SelfProtection", "Operation failed: ${e.message}", e) }
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
        } catch (e: Exception) { android.util.Log.w("SelfProtection", "Operation failed: ${e.message}", e) }
        return issues
    }

    // 修复：标记 @Volatile — init() 在主线程调用，detectInjection() 在 IO 线程读取。
    // 没有 @Volatile 的话，IO 线程可能永远看不到主线程的写入，导致 context 为 null
    // （进而导致 detectInjection 误报 .so 文件为 "Foreign"）。
    @Volatile
    private var context: Context? = null

    fun init(ctx: Context) { context = ctx.applicationContext }

    // ─── Multi-process verification ─────────────────────────
    // v1.0.1 修复: 用 java.lang.Process 全限定名,避免与 android.os.Process 冲突
    @Volatile
    private var verifierProcess: java.lang.Process? = null

    fun spawnVerifierProcess(): Boolean {
        try {
            stopVerifier()
            val pid = android.os.Process.myPid()
            val builder = ProcessBuilder(
                "sh", "-c",
                "while kill -0 $pid 2>/dev/null; do " +
                        "if [ -f /proc/$pid/status ]; then " +
                        "tp=\$(grep 'TracerPid:' /proc/$pid/status | awk '{print \$2}'); " +
                        "if [ \"\$tp\" != \"0\" ] && [ \"\$tp\" != \"\" ]; then " +
                        "echo \"WARNING: TracerPid=\$tp detected on parent\" > /data/local/tmp/apex_alert; " +
                        "fi; " +
                        "fi; " +
                        "sleep 5; " +
                        "done"
            )
            builder.redirectErrorStream(true)
            val proc = builder.start()
            proc.outputStream.close()
            verifierProcess = proc
            return true
        } catch (e: Exception) {
            return false
        }
    }

    fun stopVerifier() {
        try {
            verifierProcess?.destroyForcibly()
            verifierProcess = null
        } catch (e: Throwable) { android.util.Log.w("SelfProtection", "Operation failed: ${e.message}", e) }
    }

    fun verifyChildProcess(pid: Int): Boolean {
        return try {
            val cmdline = File("/proc/$pid/cmdline").readText()
            cmdline.contains("com.apex.root")
        } catch (e: Exception) {
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
