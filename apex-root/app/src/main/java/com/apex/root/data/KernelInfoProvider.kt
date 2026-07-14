package com.apex.root.data

import android.util.Log
import java.io.File

/**
 * 内核信息提供者 — v1.0.3 从 KernelInfoScreen.kt 提取的业务逻辑。
 *
 * 读取设备内核信息 (版本/SELinux/模块列表/构建属性)。
 * 原 UI 层代码已删除,此文件提供数据供新 UI 使用。
 */
object KernelInfoProvider {

    private const val TAG = "KernelInfo"

    data class KernelInfo(
        val kernelVersion: String,
        val architecture: String,
        val loadedModules: List<String>,
        val selinuxStatus: String,
        val securityPatch: String
    )

    fun readKernelInfo(): KernelInfo {
        return try {
            val kernelVersion = readKernelVersion()
            val arch = System.getProperty("os.arch") ?: "未知"
            val selinux = readSelinuxStatus()
            val securityPatch = readBuildProp()["ro.build.version.security_patch"]
                ?: readBuildProp()["ro.build.version.security_update"]
                ?: "未知"
            val loadedModules = readLoadedModules()
            KernelInfo(kernelVersion, arch, loadedModules, selinux, securityPatch)
        } catch (e: Throwable) {
            Log.e(TAG, "readKernelInfo failed", e)
            KernelInfo("读取失败", "未知", emptyList(), "未知", "未知")
        }
    }

    private fun readKernelVersion(): String {
        return try {
            File("/proc/version").bufferedReader().readLine() ?: "未知"
        } catch (_: Throwable) { "未知" }
    }

    fun readBuildProp(): Map<String, String> {
        val props = mutableMapOf<String, String>()
        listOf("/system/build.prop", "/vendor/build.prop", "/product/build.prop").forEach { path ->
            runCatching {
                File(path).bufferedReader().useLines { lines ->
                    lines.forEach { line ->
                        val trimmed = line.trim()
                        if (trimmed.isNotEmpty() && !trimmed.startsWith("#") && trimmed.contains("=")) {
                            val idx = trimmed.indexOf('=')
                            val k = trimmed.substring(0, idx).trim()
                            val v = trimmed.substring(idx + 1).trim()
                            if (k.isNotEmpty() && !props.containsKey(k)) props[k] = v
                        }
                    }
                }
            }
        }
        return props
    }

    fun readLoadedModules(): List<String> = runCatching {
        File("/proc/modules").bufferedReader().useLines { lines ->
            lines.mapNotNull { line ->
                val name = line.substringBefore(' ').trim()
                if (name.isNotEmpty()) name else null
            }.take(20).toList()
        }
    }.getOrDefault(emptyList())

    fun readSelinuxStatus(): String = runCatching {
        val p = Runtime.getRuntime().exec(arrayOf("getenforce"))
        val text = p.inputStream.bufferedReader().readText().trim()
        val exited = p.waitFor(2000, java.util.concurrent.TimeUnit.MILLISECONDS)
        if (exited) text else "Unknown"
    }.getOrDefault("Unknown")
}
