package com.apex.root.data

import android.util.Log
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlinx.coroutines.withTimeoutOrNull
import java.util.concurrent.TimeUnit

/**
 * Root 可用性检查器 — v1.0.3 从 GlassPermissionGuideScreen.kt 提取的业务逻辑。
 *
 * 检测设备是否已安装并授权 root 框架 (su 二进制)。
 * 带 3 秒超时,避免 su 弹窗阻塞导致 ANR。
 */
object RootAvailabilityChecker {

    private const val TAG = "RootCheck"

    /**
     * 检测 Root 状态。带 3 秒超时，避免 su 弹窗阻塞导致 ANR/卡死。
     * - 设备未安装 su：exec 抛 IOException → 返回"未安装"
     * - su 已安装但需用户授权：waitFor 阻塞 → 超时后返回"超时"
     * - su 已授权：返回 uid=0
     */
    suspend fun checkRootStatus(): Pair<Boolean, String> = withContext(Dispatchers.IO) {
        var process: Process? = null
        try {
            return@withContext withTimeoutOrNull(3000L) {
                runCatching {
                    process = Runtime.getRuntime().exec(arrayOf("su", "-c", "id"))
                    val p = process!!
                    val text = p.inputStream.bufferedReader().readText()
                    val exited = p.waitFor(3000, TimeUnit.MILLISECONDS)
                    val exit = if (exited) p.exitValue() else -1
                    if (exit == 0 && text.contains("uid=0")) {
                        true to "已获取（uid=0）"
                    } else {
                        false to "未授权（exit=$exit）"
                    }
                }.getOrElse { e ->
                    false to "未安装 root 框架（${e.message ?: e.javaClass.simpleName}）"
                }
            } ?: (false to "检测超时（su 未响应，可能需用户授权）")
        } finally {
            try { process?.destroyForcibly() } catch (_: Throwable) {}
        }
    }
}
