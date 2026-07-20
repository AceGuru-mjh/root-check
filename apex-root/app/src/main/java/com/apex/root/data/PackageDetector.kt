package com.apex.root.data

import android.content.Context
import android.content.pm.PackageInfo
import android.content.pm.PackageManager
import android.os.Build
import android.util.Log

/**
 * v1.1.1 修复 P0-D1: PackageManager 检测辅助类
 *
 * 此前的 L8/L9/L10/L21/L23 检测在 native cpp 中直接访问 `/data/adb/*` 和
 * `/data/data/<other_package>/*` 路径,但在普通 app 上下文(无 root、SELinux 限制)下
 * 这些路径永远 EACCES,导致检测 100% 失效。
 *
 * 本类用 PackageManager.getInstalledPackages() 检测已安装的 root 框架 manager app,
 * 作为 native 层路径检测的补充。AndroidManifest 已声明 QUERY_ALL_PACKAGES 权限。
 *
 * 包名清单来源: 各 root 框架官方仓库 + Google Play policy 文档 + 仅保留确定正确的包名。
 * 不确定的包名一律不加,避免误报。
 *
 * 检测覆盖 (与 native 层对应):
 *  - L8  Magisk     : com.topjohnwu.magisk (+ Delta/Alpha/Beta)
 *  - L9  KernelSU   : me.weishu.kernelsu (+ rifsxd forks / SukiSU)
 *  - L10 APatch     : me.bmax.apatch (+ rifsxd fork)
 *  - L11 Xposed     : org.lsposed.manager / lspatch / xposed installer
 *  - L23 Shizuku    : moe.shizuku.privileged.api / com.osiris0.dhizuku
 *
 * 注意: L21 (Play Integrity Bypass / TrickyStore / PIF) 通常作为 Magisk/KernelSU 模块
 * 存在,无独立 APK 包名,故不在此处检测,仍由 native 层的模块文件扫描负责。
 */
object PackageDetector {
    private const val TAG = "PackageDetector"

    // ─── L8 Magisk (官方 + 常见 fork) ─────────────────────────
    private val MAGISK_PACKAGES = setOf(
        "com.topjohnwu.magisk",            // 官方
        "io.github.huskydg.magisk",        // Magisk Delta / Huskydg
        "com.topjohnwu.magisk.alpha",      // Alpha
        "com.topjohnwu.magisk.beta"        // Beta
    )

    // ─── L9 KernelSU (官方 + rifsxd fork / SukiSU) ───────────
    private val KERNELSU_PACKAGES = setOf(
        "me.weishu.kernelsu",              // 官方
        "io.github.rifsxd.kernelsu",       // rifsxd fork
        "io.github.rifsxd.sukisu",         // SukiSU
        "io.github.rifsxd.sukisu.ultra"    // SukiSU Ultra
    )

    // ─── L10 APatch (官方 + rifsxd fork) ─────────────────────
    private val APATCH_PACKAGES = setOf(
        "me.bmax.apatch",                  // 官方
        "io.github.rifsxd.apatch"          // rifsxd fork
    )

    // ─── L23 Shizuku / Dhizuku ───────────────────────────────
    private val SHIZUKU_PACKAGES = setOf(
        "moe.shizuku.privileged.api",      // Shizuku 官方
        "com.osiris0.dhizuku"              // Dhizuku
    )

    // ─── L11 Xposed / LSPosed ────────────────────────────────
    private val XPOSED_PACKAGES = setOf(
        "org.lsposed.manager",             // LSPosed Manager
        "org.lsposed.lspatch",             // LSPatch
        "de.robv.android.xposed.installer" // 原版 Xposed Installer
    )

    /**
     * 检测结果。每个字段对应一类 root/特权框架。
     * [detectedPackages] 是命中的具体包名清单,可用于上层展示。
     */
    data class DetectionResult(
        val magisk: Boolean,
        val kernelsu: Boolean,
        val apatch: Boolean,
        val shizuku: Boolean,
        val xposed: Boolean,
        val detectedPackages: List<String>
    ) {
        /** 任一框架命中即视为已检测到 root/特权环境 */
        val anyDetected: Boolean
            get() = magisk || kernelsu || apatch || shizuku || xposed
    }

    /** 空结果 (用于异常回退) */
    private val EMPTY_RESULT = DetectionResult(
        magisk = false,
        kernelsu = false,
        apatch = false,
        shizuku = false,
        xposed = false,
        detectedPackages = emptyList()
    )

    /**
     * 使用指定 [Context] 检测已安装的 root 框架 manager app。
     * 任何异常都返回空结果,绝不抛出。
     */
    fun detect(context: Context): DetectionResult {
        val installed = try {
            queryInstalledPackages(context)
        } catch (e: Throwable) {
            Log.w(TAG, "queryInstalledPackages failed: ${e.message}")
            return EMPTY_RESULT
        }

        val magisk = MAGISK_PACKAGES.any { it in installed }
        val kernelsu = KERNELSU_PACKAGES.any { it in installed }
        val apatch = APATCH_PACKAGES.any { it in installed }
        val shizuku = SHIZUKU_PACKAGES.any { it in installed }
        val xposed = XPOSED_PACKAGES.any { it in installed }

        val detected = buildList {
            MAGISK_PACKAGES.filter { it in installed }.let(::addAll)
            KERNELSU_PACKAGES.filter { it in installed }.let(::addAll)
            APATCH_PACKAGES.filter { it in installed }.let(::addAll)
            SHIZUKU_PACKAGES.filter { it in installed }.let(::addAll)
            XPOSED_PACKAGES.filter { it in installed }.let(::addAll)
        }

        if (detected.isNotEmpty()) {
            Log.i(TAG, "Detected root/privilege packages: $detected")
        }

        return DetectionResult(
            magisk = magisk,
            kernelsu = kernelsu,
            apatch = apatch,
            shizuku = shizuku,
            xposed = xposed,
            detectedPackages = detected
        )
    }

    /**
     * 无参版本: 使用 [com.apex.root.ApexRootApplication] 保存的全局 application context。
     * 如果 Application 尚未初始化 (例如在 Application.onCreate 之前的极早期调用),返回空结果。
     */
    fun detect(): DetectionResult {
        val ctx = try {
            com.apex.root.ApexRootApplication.appContext
        } catch (e: UninitializedPropertyAccessException) {
            Log.w(TAG, "Application context not yet initialized, skip package detection")
            return EMPTY_RESULT
        } catch (e: Throwable) {
            Log.w(TAG, "Failed to get application context: ${e.message}")
            return EMPTY_RESULT
        }
        return detect(ctx)
    }

    /**
     * 枚举设备上已安装的所有包名。统一了 API 33+ 的 PackageInfoFlags.of(0) 与旧版 API 的差异。
     */
    private fun queryInstalledPackages(context: Context): Set<String> {
        val pm = context.packageManager
        val result = mutableSetOf<String>()

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            // API 33+: 必须用 PackageInfoFlags.of(0)
            val list: List<PackageInfo> = pm.getInstalledPackages(
                PackageManager.PackageInfoFlags.of(0)
            )
            list.forEach { result.add(it.packageName) }
        } else {
            // API < 33: getInstalledApplications 返回 ApplicationInfo 列表
            @Suppress("DEPRECATION")
            val list = pm.getInstalledApplications(0)
            list.forEach { result.add(it.packageName) }
        }

        return result
    }
}
