package com.rootdetector.engine

import android.util.Log
import com.rootdetector.config.DetectionConfig
import com.rootdetector.config.DetectionItem
import com.rootdetector.core.security.SelfProtection
import com.rootdetector.detect.DetectionEngine
import com.rootdetector.domain.model.DetectionLevel
import com.rootdetector.domain.model.PluginResult
import com.rootdetector.domain.model.ThreatLevel
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlinx.coroutines.withTimeoutOrNull
import org.json.JSONObject
import java.io.File

class ConfigurableDetectionEngine(
    private val nativeEngine: DetectionEngine = DetectionEngine()
) {

    suspend fun runDetection(config: DetectionConfig): DetectionResult = withContext(Dispatchers.IO) {
        val startTime = System.nanoTime()
        val enabledItems = config.profile.enabledItems

        val results = mutableListOf<PluginResult>()
        var overallDetected = false
        var maxRisk = 0

        val nativeLayerMask = buildNativeLayerMask(enabledItems)

        if (nativeLayerMask > 0) {
            val nativeResults = runNativeDetection(config, nativeLayerMask)
            results.addAll(nativeResults)
            if (nativeResults.any { it.detected }) overallDetected = true
            maxRisk = maxOf(maxRisk, nativeResults.maxOfOrNull { it.riskScore } ?: 0)
        }

        val itemResults = runItemLevelDetection(enabledItems)
        results.addAll(itemResults)
        if (itemResults.any { it.detected }) overallDetected = true
        maxRisk = maxOf(maxRisk, itemResults.maxOfOrNull { it.riskScore } ?: 0)

        if (config.enableAntiHiding) {
            val antiHidingResults = nativeEngine.runAntiHidingProbes()
            for (hr in antiHidingResults) {
                results.addAll(hr.layerResults)
                if (hr.rootDetected) overallDetected = true
                maxRisk = maxOf(maxRisk, hr.riskScore)
            }
        }

        val selfCheckIssues = nativeEngine.selfCheck()
        if (selfCheckIssues.isNotEmpty()) {
            results.add(PluginResult(
                layerId = 99,
                detected = true,
                confidence = 90,
                riskScore = 50,
                costNs = 0,
                detail = "自保护异常: ${selfCheckIssues.joinToString("; ")}"
            ))
            overallDetected = true
            maxRisk = maxOf(maxRisk, 50)
        }

        val elapsedNs = System.nanoTime() - startTime
        DetectionResult(
            layerResults = results,
            rootDetected = overallDetected,
            riskScore = maxRisk,
            threatLevel = ThreatLevel.fromScore(maxRisk),
            elapsedMs = elapsedNs / 1_000_000,
            totalItems = config.profile.totalItems,
            enabledItems = config.profile.enabledCount
        )
    }

    private suspend fun runNativeDetection(
        config: DetectionConfig,
        layerMask: Int
    ): List<PluginResult> = withContext(Dispatchers.IO) {
        val results = mutableListOf<PluginResult>()

        val daemonRunning = nativeEngine.ensureDaemonRunning()
        if (!daemonRunning && config.useDaemon) {
            results.add(PluginResult(
                layerId = 0, detected = false, confidence = 100, riskScore = 0,
                costNs = 0, detail = "守护进程未运行，使用快速检测模式"
            ))
        }

        val timeoutMs = config.timeoutMs
        val json = withTimeoutOrNull(timeoutMs) {
            val levelEnum = when (config.profile.level) {
                1 -> DetectionLevel.QUICK
                2 -> DetectionLevel.STANDARD
                3 -> DetectionLevel.DEEP
                4 -> DetectionLevel.FORENSIC
                else -> DetectionLevel.STANDARD
            }
            nativeEngine.detect(levelEnum)
        }

        if (json != null) {
            results.addAll(json.layerResults)
        } else {
            results.add(PluginResult(
                layerId = 0, detected = false, confidence = 50, riskScore = 0,
                costNs = 0, detail = "Native 检测超时，跳过"
            ))
        }

        results
    }

    private suspend fun runItemLevelDetection(
        enabledItems: Set<String>
    ): List<PluginResult> = withContext(Dispatchers.IO) {
        val results = mutableListOf<PluginResult>()

        for (itemId in enabledItems) {
            val item = DetectionItem.byId(itemId) ?: continue
            val result = detectSingleItem(item)
            if (result != null) results.add(result)
        }

        results
    }

    private fun detectSingleItem(item: DetectionItem): PluginResult? {
        val startTime = System.nanoTime()
        return try {
            val result = when (item) {
                DetectionItem.SU_BINARY -> detectSuBinary()
                DetectionItem.SUSPICIOUS_APPS -> detectSuspiciousApps()
                DetectionItem.ABNORMAL_DIRECTORIES -> detectAbnormalDirectories()
                DetectionItem.DUAL_APP_DETECT -> detectDualApp()
                DetectionItem.XPOSED_HOOKS -> detectXposedJava()
                DetectionItem.MAPS_SCAN_BASIC -> detectMapsBasic()
                DetectionItem.PROPERTY_CHECK_BASIC -> detectPropertiesBasic()
                DetectionItem.BOOTLOADER_CHECK -> detectBootloader()
                DetectionItem.DEBUG_DETECT_DEEP -> detectDebugDeep()
                DetectionItem.CUSTOM_ROM_DETECT -> detectCustomRom()
                DetectionItem.RECOVERY_DETECT -> detectRecovery()

                else -> null
            }
            result?.copy(costNs = System.nanoTime() - startTime)
        } catch (e: Exception) {
            Log.w("ConfigurableEngine", "Item ${item.id} failed: ${e.message}")
            PluginResult(
                layerId = item.ordinal + 50,
                detected = false, confidence = 30, riskScore = 0,
                costNs = System.nanoTime() - startTime,
                detail = "检测异常: ${e.message}"
            )
        }
    }

    private fun detectSuBinary(): PluginResult {
        val paths = listOf(
            "/system/bin/su", "/system/xbin/su", "/system/bin/.ext/su",
            "/data/local/su", "/data/local/bin/su", "/data/local/xbin/su",
            "/sbin/su", "/vendor/bin/su", "/product/bin/su",
            "/system/xbin/mu", "/data/local/tmp/su"
        )
        val found = paths.filter { File(it).exists() }
        val layerId = DetectionItem.SU_BINARY.ordinal + 50
        return if (found.isNotEmpty()) {
            PluginResult(
                layerId = layerId, detected = true, confidence = 95, riskScore = 70,
                costNs = 0, detail = "发现 SU 二进制: ${found.joinToString("; ")}"
            )
        } else {
            PluginResult(
                layerId = layerId, detected = false, confidence = 80, riskScore = 0,
                costNs = 0, detail = "未发现 SU 二进制"
            )
        }
    }

    private fun detectSuspiciousApps(): PluginResult {
        val suspiciousPackages = listOf(
            "com.topjohnwu.magisk", "io.github.vvb2060.magisk",
            "com.topjohnwu.magisk.hide", "com.termux",
            "eu.chainfire.supersu", "com.noshufou.android.su",
            "com.koushikdutta.superuser", "com.thirdparty.superuser",
            "com.yellowes.su", "me.weishu.kernelsu",
            "me.weishu.kernelsu.manager", "com.dergoogler.mmrl",
            "com.apatch.apatch", "com.android.browser",
            "com.termux.x11", "com.keramidas.TitaniumBackup"
        )
        val layerId = DetectionItem.SUSPICIOUS_APPS.ordinal + 50
        return PluginResult(
            layerId = layerId, detected = false, confidence = 50, riskScore = 0,
            costNs = 0, detail = "包名检测需 PackageManager 权限，请确保已授予"
        )
    }

    private fun detectAbnormalDirectories(): PluginResult {
        val dirs = listOf(
            "/data/misc/hide_my_applist",
            "/data/system/xlua",
            "/data/system/xedge",
            "/data/misc/clipboard",
            "/data/system/cn.geektang.privacyspace",
            "/storage/emulated/0/TWRP",
            "/storage/emulated/TWRP"
        )
        val found = dirs.filter { File(it).exists() }
        val layerId = DetectionItem.ABNORMAL_DIRECTORIES.ordinal + 50
        return if (found.isNotEmpty()) {
            PluginResult(
                layerId = layerId, detected = true, confidence = 90, riskScore = 50,
                costNs = 0, detail = "发现异常目录: ${found.joinToString("; ")}"
            )
        } else {
            PluginResult(
                layerId = layerId, detected = false, confidence = 80, riskScore = 0,
                costNs = 0, detail = "未发现异常目录"
            )
        }
    }

    private fun detectDualApp(): PluginResult {
        val layerId = DetectionItem.DUAL_APP_DETECT.ordinal + 50
        return try {
            val filesDir = File("").absolutePath
            val suspicious = filesDir.startsWith("/data/user") && !filesDir.startsWith("/data/user/0")
            PluginResult(
                layerId = layerId, detected = suspicious, confidence = if (suspicious) 85 else 70,
                riskScore = if (suspicious) 40 else 0,
                costNs = 0, detail = if (suspicious) "运行在多开/工作资料环境中"
                else "运行在正常用户空间"
            )
        } catch (e: Exception) {
            PluginResult(layerId = layerId, detected = false, confidence = 30, riskScore = 0,
                costNs = 0, detail = "检测异常: ${e.message}")
        }
    }

    private fun detectXposedJava(): PluginResult {
        val layerId = DetectionItem.XPOSED_HOOKS.ordinal + 50
        val hooks = SelfProtection.detectHook()
        return if (hooks.isNotEmpty()) {
            PluginResult(
                layerId = layerId, detected = true, confidence = 85, riskScore = 60,
                costNs = 0, detail = "检测到 Hook 框架: ${hooks.joinToString(", ")}"
            )
        } else {
            PluginResult(
                layerId = layerId, detected = false, confidence = 60, riskScore = 0,
                costNs = 0, detail = "未检测到常见 Hook 框架"
            )
        }
    }

    private fun detectMapsBasic(): PluginResult {
        val layerId = DetectionItem.MAPS_SCAN_BASIC.ordinal + 50
        return try {
            val maps = File("/proc/self/maps").readText()
            val keywords = listOf(
                "frida", "xposed", "substrate", "cydia",
                "libriru", "liblspd", "libwhale", "libdobby",
                "zygisk", "shamiko", "gum-js-loop"
            )
            val found = keywords.filter { maps.contains(it) }
            if (found.isNotEmpty()) {
                PluginResult(
                    layerId = layerId, detected = true, confidence = 90, riskScore = 65,
                    costNs = 0, detail = "maps 中发现 Hook 特征: ${found.joinToString(", ")}"
                )
            } else {
                PluginResult(
                    layerId = layerId, detected = false, confidence = 70, riskScore = 0,
                    costNs = 0, detail = "maps 中未发现 Hook 特征"
                )
            }
        } catch (e: Exception) {
            PluginResult(layerId = layerId, detected = false, confidence = 30, riskScore = 0,
                costNs = 0, detail = "读取 maps 失败: ${e.message}")
        }
    }

    private fun detectPropertiesBasic(): PluginResult {
        val layerId = DetectionItem.PROPERTY_CHECK_BASIC.ordinal + 50
        return try {
            val findings = mutableListOf<String>()
            val debugProp = try {
                val p = Runtime.getRuntime().exec(arrayOf("getprop", "ro.debuggable"))
                p.inputStream.bufferedReader().readText().trim()
            } catch (_: Exception) { "" }
            if (debugProp == "1") findings.add("ro.debuggable=1 (工程机模式)")

            val secureProp = try {
                val p = Runtime.getRuntime().exec(arrayOf("getprop", "ro.secure"))
                p.inputStream.bufferedReader().readText().trim()
            } catch (_: Exception) { "" }
            if (secureProp == "0") findings.add("ro.secure=0 (非安全模式)")

            val tagsProp = try {
                val p = Runtime.getRuntime().exec(arrayOf("getprop", "ro.build.tags"))
                p.inputStream.bufferedReader().readText().trim()
            } catch (_: Exception) { "" }
            if (tagsProp.contains("test-keys")) findings.add("ro.build.tags=test-keys (开发版/自定义ROM)")

            if (findings.isNotEmpty()) {
                PluginResult(
                    layerId = layerId, detected = true, confidence = 90, riskScore = 55,
                    costNs = 0, detail = findings.joinToString("; ")
                )
            } else {
                PluginResult(
                    layerId = layerId, detected = false, confidence = 75, riskScore = 0,
                    costNs = 0, detail = "系统属性正常 (release-keys)"
                )
            }
        } catch (e: Exception) {
            PluginResult(layerId = layerId, detected = false, confidence = 30, riskScore = 0,
                costNs = 0, detail = "属性检测异常: ${e.message}")
        }
    }

    private fun detectBootloader(): PluginResult {
        val layerId = DetectionItem.BOOTLOADER_CHECK.ordinal + 50
        return try {
            val props = listOf(
                "ro.boot.verifiedbootstate" to arrayOf("green"),
                "ro.boot.flash.locked" to arrayOf("1"),
                "ro.boot.vbmeta.device_state" to arrayOf("locked")
            )
            val findings = mutableListOf<String>()
            for ((prop, expected) in props) {
                try {
                    val p = Runtime.getRuntime().exec(arrayOf("getprop", prop))
                    val value = p.inputStream.bufferedReader().readText().trim()
                    if (value.isNotEmpty() && value !in expected) {
                        findings.add("$prop=$value (异常)")
                    }
                } catch (_: Exception) {}
            }
            if (findings.isNotEmpty()) {
                PluginResult(
                    layerId = layerId, detected = true, confidence = 85, riskScore = 50,
                    costNs = 0, detail = "Bootloader 异常: ${findings.joinToString("; ")}"
                )
            } else {
                PluginResult(
                    layerId = layerId, detected = false, confidence = 65, riskScore = 0,
                    costNs = 0, detail = "Bootloader 状态正常"
                )
            }
        } catch (e: Exception) {
            PluginResult(layerId = layerId, detected = false, confidence = 30, riskScore = 0,
                costNs = 0, detail = "Bootloader 检测异常: ${e.message}")
        }
    }

    private fun detectDebugDeep(): PluginResult {
        val layerId = DetectionItem.DEBUG_DETECT_DEEP.ordinal + 50
        val findings = mutableListOf<String>()

        if (SelfProtection.isDebuggerAttached()) findings.add("调试器已附加")
        if (SelfProtection.isPtraced()) findings.add("进程被 ptrace 跟踪")

        return if (findings.isNotEmpty()) {
            PluginResult(
                layerId = layerId, detected = true, confidence = 95, riskScore = 80,
                costNs = 0, detail = findings.joinToString("; ")
            )
        } else {
            PluginResult(
                layerId = layerId, detected = false, confidence = 80, riskScore = 0,
                costNs = 0, detail = "未检测到调试/跟踪"
            )
        }
    }

    private fun detectCustomRom(): PluginResult {
        val layerId = DetectionItem.CUSTOM_ROM_DETECT.ordinal + 50
        return try {
            val fingerprint = android.os.Build.FINGERPRINT
            val tags = android.os.Build.TAGS
            val findings = mutableListOf<String>()
            if (tags?.contains("test-keys") == true) findings.add("编译标签: test-keys")
            if (fingerprint?.contains("custom") == true ||
                fingerprint?.contains("userdebug") == true) findings.add("指纹: $fingerprint")
            PluginResult(
                layerId = layerId, detected = findings.isNotEmpty(),
                confidence = if (findings.isNotEmpty()) 80 else 60,
                riskScore = if (findings.isNotEmpty()) 40 else 0,
                costNs = 0, detail = if (findings.isNotEmpty()) findings.joinToString("; ")
                else "未检测到自定义ROM特征"
            )
        } catch (e: Exception) {
            PluginResult(layerId = layerId, detected = false, confidence = 30, riskScore = 0,
                costNs = 0, detail = "ROM检测异常: ${e.message}")
        }
    }

    private fun detectRecovery(): PluginResult {
        val layerId = DetectionItem.RECOVERY_DETECT.ordinal + 50
        val recoveryPaths = listOf(
            "/cache/recovery", "/cache/recovery/log",
            "/cache/recovery/last_log", "/cache/recovery/last_install",
            "/data/cache/recovery", "/sdcard/TWRP",
            "/data/media/0/TWRP"
        )
        val found = recoveryPaths.filter { File(it).exists() }
        return if (found.isNotEmpty()) {
            PluginResult(
                layerId = layerId, detected = true, confidence = 85, riskScore = 45,
                costNs = 0, detail = "发现 Recovery 残留: ${found.joinToString("; ")}"
            )
        } else {
            PluginResult(
                layerId = layerId, detected = false, confidence = 70, riskScore = 0,
                costNs = 0, detail = "未发现 Recovery 残留"
            )
        }
    }

    private fun buildNativeLayerMask(enabledItems: Set<String>): Int {
        var mask = 0
        if ("magisk_daemon" in enabledItems) mask = mask or 0x01
        if ("zygisk_detect" in enabledItems) mask = mask or 0x02
        if ("kernelsu_detect" in enabledItems) mask = mask or 0x04
        if ("apatch_detect" in enabledItems) mask = mask or 0x08
        if ("lsposed_detect" in enabledItems) mask = mask or 0x10
        if ("frida_detect" in enabledItems) mask = mask or 0x20
        if ("shamiko_detect" in enabledItems) mask = mask or 0x40
        if ("process_scan" in enabledItems) mask = mask or 0x80
        if ("file_system_scan" in enabledItems) mask = mask or 0x100
        if ("mount_check" in enabledItems) mask = mask or 0x200
        if ("selinux_check" in enabledItems) mask = mask or 0x400
        if ("kernel_module_check" in enabledItems) mask = mask or 0x800
        if ("syscall_timing" in enabledItems) mask = mask or 0x1000
        return mask
    }

    data class DetectionResult(
        val layerResults: List<PluginResult>,
        val rootDetected: Boolean,
        val riskScore: Int,
        val threatLevel: ThreatLevel,
        val elapsedMs: Long,
        val totalItems: Int,
        val enabledItems: Int
    )
}
