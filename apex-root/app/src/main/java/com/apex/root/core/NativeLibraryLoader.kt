package com.apex.root.core

import android.util.Log

/**
 * 统一的 Native 库加载器。
 * 所有需要加载 libapex_root.so 的地方都应通过此对象加载，
 * 避免重复加载和未捕获的 UnsatisfiedLinkError 崩溃。
 */
object NativeLibraryLoader {
    const val TAG = "NativeLoader"
    const val LIB_NAME = "apex_root"

    @Volatile
    var loaded = false

    @Volatile
    var loadFailed = false

    /**
     * 安全加载 native 库。如果加载失败，不会抛出异常，而是标记为失败。
     * 后续调用 native 方法时会抛出 UnsatisfiedLinkError，
     * 调用方应在 ViewModel 层 try-catch 处理。
     */
    fun ensureLoaded() {
        if (loaded || loadFailed) return
        synchronized(this) {
            if (loaded || loadFailed) return
            try {
                System.loadLibrary(LIB_NAME)
                loaded = true
                Log.i(TAG, "Native library '$LIB_NAME' loaded successfully")
            } catch (e: UnsatisfiedLinkError) {
                loadFailed = true
                Log.e(TAG, "Failed to load native library '$LIB_NAME'", e)
            } catch (e: Exception) {
                loadFailed = true
                Log.e(TAG, "Unexpected error loading native library '$LIB_NAME'", e)
            }
        }
    }

    /** 检查 native 库是否已成功加载 */
    val isAvailable: Boolean get() = loaded

    /**
     * 安全执行 native 调用。如果库未加载，返回默认值而不是崩溃。
     * @param default 库未加载时的默认返回值
     * @param block 实际的 native 调用
     */
    inline fun <T> safeCall(default: T, block: () -> T): T {
        return try {
            ensureLoaded()
            if (!loaded) default else block()
        } catch (e: UnsatisfiedLinkError) {
            Log.e(TAG, "Native method not found", e)
            default
        } catch (e: Exception) {
            Log.e(TAG, "Native call failed", e)
            default
        }
    }

    /**
     * 安全执行无返回值的 native 调用
     */
    inline fun safeRun(block: () -> Unit) {
        try {
            ensureLoaded()
            if (loaded) block()
        } catch (e: UnsatisfiedLinkError) {
            Log.e(TAG, "Native method not found", e)
        } catch (e: Exception) {
            Log.e(TAG, "Native call failed", e)
        }
    }
}
