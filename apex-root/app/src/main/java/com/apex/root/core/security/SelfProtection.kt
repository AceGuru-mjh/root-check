package com.apex.root.core.security

import android.content.Context
import android.content.pm.PackageManager
import android.os.Process
import java.io.File
import java.security.MessageDigest
import java.util.zip.Adler32

object SelfProtection {
    // P0-K2 / 2-c#10 修复 (v1.1.1): 以下可变状态跨主线程与 IO 线程访问, 必须加 @Volatile
    // (context / verifierProcess 已加, 修复 verified / apkHash / lastIntegrityCheck 三个道漏)
    @Volatile
    private var verified = false
    @Volatile
    private var apkHash: String = ""
    @Volatile
    private var lastIntegrityCheck = 0L
    private const val INTEGRITY_CHECK_INTERVAL = 30000L // 30 seconds

    /**
     * P0-S2 修复 (v1.1.1): 预期签名证书 SHA-256 指纹白名单。
     *
     * 之前 verifyApkIntegrity 只校验 APK 有签名, 不校验签名者身份, 导致任意重签名 APK
     * 都能通过校验。现在改为: 计算实际签名证书 SHA-256, 与白名单比对, 不匹配则拒绝。
     *
     * 配置指南:
     *  - debug 构建: 由调用者确保 BuildConfig.DEBUG=true 时跳过白名单严格校验
     *    (debug keystore 每台机器不同)
     *  - release 构建: 需在构建时通过 build.gradle.kts buildConfigField 注入实际
     *    release 签名指纹。这里提供两个示例指纹 (空字符串), release 会走跳过逻辑。
     *
     * TODO(P0): 把实际 release 证书指纹通过 BuildConfig.APEX_SIGNING_SHA256 注入后,
     * 这里改为 BuildConfig.APEX_SIGNING_SHA256。
     */
    private val EXPECTED_SIGNING_CERT_SHA256: Set<String> = buildSet {
        // 预留: release 签名指纹 (64 hex chars, 小写, 无冒号)
        // 例: add("abcdef0123456789...")
    }

    /**
     * 当 EXPECTED_SIGNING_CERT_SHA256 为空时, 退化为只校验 "有签名" (与 v1.0.3 同行为)。
     * 用于避免在 release 证书指纹未配置时 阻碍开发。
     *
     * 一旦在 build.gradle.kts 中注入 EXPECTED_SIGNING_CERT_SHA256, 此标志应改为 false
     * (或以 BuildConfig.DEBUG 动态控制)。
     */
    private const val ALLOW_FALLBACK_WHEN_NO_WHITELIST: Boolean = true

    // ─── APK integrity verification ─────────────────────────
    // P0-S2 修复 (v1.1.1): 旧实现只校验 signatures[0] 是否存在, 不比对哈希白名单,
    // 攻击者可任意重签名 APK 通过校验。现在改为严格比对签名证书 SHA-256。
    fun verifyApkIntegrity(context: Context): Boolean {
        try {
            // Android P+ 推荐使用 GET_SIGNING_CERTIFICATES (包含 signingInfo),
            // 但同时保留 GET_SIGNATURES 兑现于 Android P 以下。
            val info = if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.P) {
                context.packageManager.getPackageInfo(
                    context.packageName,
                    PackageManager.GET_SIGNING_CERTIFICATES
                )
            } else {
                @Suppress("DEPRECATION")
                context.packageManager.getPackageInfo(
                    context.packageName,
                    PackageManager.GET_SIGNATURES
                )
            }

            // 优先走 signingInfo.apkContentsSigners (P+), fallback 到 signatures
            val sigs: Array<out android.content.pm.Signature>? =
                if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.P) {
                    info.signingInfo?.apkContentsSigners
                } else {
                    @Suppress("DEPRECATION")
                    info.signatures
                }

            if (sigs.isNullOrEmpty()) {
                android.util.Log.w("SelfProtection", "verifyApkIntegrity: no signatures found")
                verified = false
                return false
            }

            // 取第一个签名者计算 SHA-256
            val sig = sigs[0].toByteArray()
            val digest = MessageDigest.getInstance("SHA-256").digest(sig)
            apkHash = digest.joinToString("") { "%02x".format(it) }

            // P0-S2: 严格比对白名单。若白名单为空且 ALLOW_FALLBACK_WHEN_NO_WHITELIST=true,
            // 退化接受 (但记日志告警, 提醒要尽快注入证书指纹)。
            val matched = EXPECTED_SIGNING_CERT_SHA256.contains(apkHash)
            if (matched) {
                verified = true
                return true
            }
            if (EXPECTED_SIGNING_CERT_SHA256.isEmpty() && ALLOW_FALLBACK_WHEN_NO_WHITELIST) {
                android.util.Log.w(
                    "SelfProtection",
                    "verifyApkIntegrity: signing whitelist is EMPTY — " +
                        "falling back to weak verification (only checks presence of signature). " +
                        "P0 security risk: any resigned APK will pass. " +
                        "Configure BuildConfig.APEX_SIGNING_SHA256 to enforce strict verification."
                )
                verified = true
                return true
            }
            // 白名单不匹配且未允许 fallback → 拒绝
            android.util.Log.e(
                "SelfProtection",
                "verifyApkIntegrity: signing cert SHA-256 mismatch! " +
                    "expected one of $EXPECTED_SIGNING_CERT_SHA256, got $apkHash"
            )
            verified = false
            return false
        } catch (e: Exception) {
            android.util.Log.w("SelfProtection", "verifyApkIntegrity failed: ${e.message}", e)
            verified = false
            return false
        }
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
            // v1.1.3 P2-S15: 扩展合法 DEX 路径白名单, 避免误报 ART 编译缓存等
            // ─────────────────────────────────────────────────────────────
            // 此前 knownPrefixes 只有 /data/app/ 和 /system/framework/,
            // 但 ART 也在 /data/dalvik-cache/ 加载 .odex/.oat, 会被误报为
            // "Unexpected DEX location"。补充合法路径:
            //   - /data/dalvik-cache/  ART 编译缓存 (boot.art/VEX/etc.)
            //   - /apex/                APEX 模块 (Android 10+)
            //   - /system/system_ext/   system_ext 分区
            //   - /system/product/      product 分区
            // ─────────────────────────────────────────────────────────────
            val knownPrefixes = listOf(
                "/data/app/",            // 用户安装的 APK
                "/system/framework/",    // 系统框架
                "/data/dalvik-cache/",   // ART 编译缓存 (P2-S15: 避免误报)
                "/apex/",                // APEX 模块 (Android 10+)
                "/system/system_ext/",   // system_ext 分区
                "/system/product/"       // product 分区
            )
            for (line in dexLines) {
                val path = line.substringAfterLast(" ").trim()
                if (path.isNotEmpty() && knownPrefixes.none { path.startsWith(it) }) {
                    issues.add("Unexpected DEX location: $path")
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
            // v1.1.3 P2-S14: 告警文件从 /data/local/tmp/ 改为 context.filesDir
            // ─────────────────────────────────────────────────────────────
            // /data/local/tmp 由 shell 用户拥有, app (UID 10xxx) 无写权,
            // 即使检测到 TracerPid!=0, 告警也无法持久化。
            // 改用 context.filesDir (app 内部存储, 有写权)。
            // 注意: init(ctx) 必须先调用, 否则 context 为 null, 退化为不写文件只检测。
            // ─────────────────────────────────────────────────────────────
            val ctx = context
            val alertFile = if (ctx != null) {
                File(ctx.filesDir, "apex_alert.log").absolutePath
            } else {
                // context 未初始化 — 无法持久化告警, 但 shell 脚本仍运行做检测
                "/dev/null"
            }
            val builder = ProcessBuilder(
                "sh", "-c",
                "while kill -0 $pid 2>/dev/null; do " +
                        "if [ -f /proc/$pid/status ]; then " +
                        "tp=\$(grep 'TracerPid:' /proc/$pid/status | awk '{print \$2}'); " +
                        "if [ \"\$tp\" != \"0\" ] && [ \"\$tp\" != \"\" ]; then " +
                        "echo \"WARNING: TracerPid=\$tp detected on parent\" > '$alertFile'; " +
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
