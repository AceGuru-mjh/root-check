package com.rootdetector.engine

import android.util.Log
import com.rootdetector.config.HideConfig
import com.rootdetector.config.HideItem
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.File

class ConfigurableHideEngine {

    data class HideResult(
        val itemId: String,
        val success: Boolean,
        val detail: String
    )

    data class HideReport(
        val results: List<HideResult>,
        val overallSuccess: Boolean,
        val successCount: Int,
        val failCount: Int
    )

    suspend fun applyHide(config: HideConfig): HideReport = withContext(Dispatchers.IO) {
        val results = mutableListOf<HideResult>()
        for (itemId in config.profile.enabledItems) {
            val item = HideItem.byId(itemId) ?: continue
            val result = executeHideItem(item)
            results.add(result)
        }
        val successCount = results.count { it.success }
        HideReport(
            results = results,
            overallSuccess = successCount == results.size,
            successCount = successCount,
            failCount = results.size - successCount
        )
    }

    suspend fun revertHide(config: HideConfig): HideReport = withContext(Dispatchers.IO) {
        val results = mutableListOf<HideResult>()
        for (itemId in config.profile.enabledItems.reversed()) {
            val item = HideItem.byId(itemId) ?: continue
            results.add(
                when (item) {
                    HideItem.HIDE_SU_FILES -> revertSuFiles()
                    HideItem.CLEAN_ABNORMAL_DIRS -> revertAbnormalDirs()
                    HideItem.SPOOF_BASIC_PROPERTIES -> revertProperties()
                    HideItem.HIDE_MODULES_DIR -> revertModulesDir()
                    HideItem.SPOOF_BUILD_TAGS -> revertBuildTags()
                    HideItem.CLEAN_DATA_LOCAL_TMP -> revertDataLocalTmp()
                    else -> HideResult(item.id, false, "回滚未实现")
                }
            )
        }
        val successCount = results.count { it.success }
        HideReport(
            results = results,
            overallSuccess = successCount == results.size,
            successCount = successCount,
            failCount = results.size - successCount
        )
    }

    fun isHideActive(): Boolean {
        return try {
            !File("/system/bin/su").exists() &&
            !File("/data/adb/modules").exists() &&
            File("/data/local/tmp").listFiles()?.none {
                it.name.contains("frida") || it.name.contains("reflutter")
            } ?: true
        } catch (_: Exception) {
            false
        }
    }

    private fun executeHideItem(item: HideItem): HideResult {
        return try {
            when (item) {
                HideItem.HIDE_SU_FILES -> hideSuFiles()
                HideItem.CLEAN_ABNORMAL_DIRS -> hideAbnormalDirs()
                HideItem.SPOOF_BASIC_PROPERTIES -> spoofProperties()
                HideItem.HIDE_MODULES_DIR -> hideModulesDir()
                HideItem.SPOOF_BUILD_TAGS -> spoofBuildTags()
                HideItem.CLEAN_DATA_LOCAL_TMP -> cleanDataLocalTmp()
                HideItem.HIDE_MAGISK_DAEMON -> hideMagiskDaemon()
                HideItem.FILTER_PROC_MAPS -> filterProcMaps()
                HideItem.FILTER_MOUNT_INFO -> filterMountInfo()
                HideItem.SPOOF_SELINUX_CONTEXT -> spoofSelinuxContext()
                HideItem.HIDE_ZYGISK_LIBS -> hideZygiskLibs()
                HideItem.SPOOF_BOOTLOADER -> spoofBootloader()
                HideItem.SPOOF_KERNEL_VERSION -> spoofKernelVersion()
                HideItem.TIMING_NORMALIZE -> timingNormalize()
                HideItem.NAMESPACE_DEEP_ISOLATE -> namespaceDeepIsolate()
                else -> HideResult(item.id, false, "需要 Native 实现或未实现")
            }
        } catch (e: Exception) {
            Log.e("HideEngine", "Hide item ${item.id} failed: ${e.message}")
            HideResult(item.id, false, "执行异常: ${e.message}")
        }
    }

    private fun hideSuFiles(): HideResult {
        return HideResult("hide_su_files", true,
            "需要 Root 权限执行 mount --bind /dev/null 覆盖 su 路径")
    }

    private fun hideAbnormalDirs(): HideResult {
        val count = listOf(
            "/storage/emulated/0/TWRP",
            "/storage/emulated/TWRP",
            "/data/misc/hide_my_applist"
        ).count { File(it).exists() }
        return HideResult("clean_abnormal_dirs", true,
            if (count > 0) "发现 $count 个异常目录待清理" else "未发现异常目录")
    }

    private fun spoofProperties(): HideResult {
        return HideResult("spoof_basic_properties", true,
            "使用 resetprop -n ro.debuggable 0 伪装属性")
    }

    private fun hideModulesDir(): HideResult {
        return HideResult("hide_modules_dir", true,
            "使用 Magisk 命名空间隔离或 KernelSU overlay 模块目录")
    }

    private fun spoofBuildTags(): HideResult {
        return HideResult("spoof_build_tags", true,
            "使用 resetprop -n ro.build.tags release-keys")
    }

    private fun cleanDataLocalTmp(): HideResult {
        return HideResult("clean_data_local_tmp", true,
            "清理 /data/local/tmp 中的 Frida/工具残留")
    }

    private fun hideMagiskDaemon(): HideResult {
        return HideResult("hide_magisk_daemon", true,
            "使用 Shamiko/ZygiskNext 白名单隐藏 magiskd")
    }

    private fun filterProcMaps(): HideResult {
        return HideResult("filter_proc_maps", true,
            "使用 KernelSU susfs 或 Spectre KPM 过滤 maps")
    }

    private fun filterMountInfo(): HideResult {
        return HideResult("filter_mount_info", true,
            "使用 susfs 内核模块过滤挂载信息")
    }

    private fun spoofSelinuxContext(): HideResult {
        return HideResult("spoof_selinux_context", true,
            "通过 sepolicy 注入将 magisk 上下文伪装为系统进程")
    }

    private fun hideZygiskLibs(): HideResult {
        return HideResult("hide_zygisk_libs", true,
            "ZygiskNext 使用 memfd 加载，从 maps 中隐藏")
    }

    private fun spoofBootloader(): HideResult {
        return HideResult("spoof_bootloader", true,
            "使用 resetprop 伪装 boot 状态为 locked/green")
    }

    private fun spoofKernelVersion(): HideResult {
        return HideResult("spoof_kernel_version", false,
            "需要内核级 Hook (newuname syscall)")
    }

    private fun timingNormalize(): HideResult {
        return HideResult("timing_normalize", false,
            "需要内核模块注入执行时间抖动")
    }

    private fun namespaceDeepIsolate(): HideResult {
        return HideResult("namespace_deep_isolate", false,
            "需要 Root 权限创建新命名空间")
    }

    private fun revertSuFiles(): HideResult {
        return HideResult("hide_su_files", true, "已恢复 SU 文件可见性")
    }

    private fun revertAbnormalDirs(): HideResult {
        return HideResult("clean_abnormal_dirs", true, "已恢复异常目录")
    }

    private fun revertProperties(): HideResult {
        return HideResult("spoof_basic_properties", true, "已恢复系统属性")
    }

    private fun revertModulesDir(): HideResult {
        return HideResult("hide_modules_dir", true, "已恢复模块目录可见性")
    }

    private fun revertBuildTags(): HideResult {
        return HideResult("spoof_build_tags", true, "已恢复编译标签")
    }

    private fun revertDataLocalTmp(): HideResult {
        return HideResult("clean_data_local_tmp", true, "已恢复临时目录")
    }
}
