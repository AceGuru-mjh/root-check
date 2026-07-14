package com.apex.root.core.error

/**
 * 统一业务错误密封类
 * 分类处理所有业务错误，支持自动重试策略
 */
sealed class AppError : Throwable() {

    /** 原生库未加载 */
    data class NativeLibraryNotAvailable(
        override val message: String = "原生库未加载，部分功能不可用"
    ) : AppError()

    /** 需要 Root 权限 */
    data class RootRequired(
        val feature: String,
        override val message: String = "$feature 需要 Root 权限"
    ) : AppError()

    /** IPC 通信错误 */
    data class IpcError(
        override val message: String = "IPC 通信失败",
        val socketName: String? = null
    ) : AppError()

    /** 权限不足 */
    data class PermissionDenied(
        val permission: String,
        override val message: String = "缺少权限: $permission"
    ) : AppError()

    /** 缓存过期 */
    data class CacheExpired(
        override val message: String = "缓存已过期，请重新扫描"
    ) : AppError()

    /** 网络错误 */
    data class NetworkError(
        override val message: String = "网络连接失败",
        val originalCause: Throwable? = null
    ) : AppError()

    /** 未知错误 */
    data class Unknown(
        override val message: String = "未知错误",
        val originalCause: Throwable? = null
    ) : AppError()

    /**
     * 用户友好消息（用于 UI 显示）
     */
    val userMessage: String
        get() = when (this) {
            is NativeLibraryNotAvailable -> "应用原生库未加载，检测功能不可用。请确认设备架构为 ARM64。"
            is RootRequired -> "「$feature」需要 Root 权限才能使用。"
            is IpcError -> "守护进程通信失败，请重启应用。"
            is PermissionDenied -> "需要授予 $permission 权限。"
            is CacheExpired -> "数据已过期，请重新扫描。"
            is NetworkError -> "网络连接异常，请检查网络后重试。"
            is Unknown -> "操作失败，请稍后重试。"
        }

    /**
     * 是否可重试
     */
    val isRetryable: Boolean
        get() = when (this) {
            is NetworkError, is IpcError, is CacheExpired -> true
            is NativeLibraryNotAvailable, is RootRequired,
            is PermissionDenied, is Unknown -> false
        }

    /**
     * 建议重试延迟（毫秒）
     */
    val retryDelayMs: Long
        get() = when (this) {
            is NetworkError -> 3000L
            is IpcError -> 2000L
            is CacheExpired -> 0L
            else -> 0L
        }
}

/**
 * 统一结果类型
 */
sealed class AppResult<out T> {
    data class Success<T>(val data: T) : AppResult<T>()
    data class Failure(val error: AppError) : AppResult<Nothing>()

    inline fun <R> map(transform: (T) -> R): AppResult<R> = when (this) {
        is Success -> Success(transform(data))
        is Failure -> this
    }

    inline fun onSuccess(action: (T) -> Unit): AppResult<T> {
        if (this is Success) action(data)
        return this
    }

    inline fun onFailure(action: (AppError) -> Unit): AppResult<T> {
        if (this is Failure) action(error)
        return this
    }

    fun getOrNull(): T? = (this as? Success)?.data
    fun errorOrNull(): AppError? = (this as? Failure)?.error
    val isSuccess: Boolean get() = this is Success
    val isFailure: Boolean get() = this is Failure

    companion object {
        fun <T> success(data: T): AppResult<T> = Success(data)
        fun failure(error: AppError): AppResult<Nothing> = Failure(error)

        /**
         * 从异常创建 AppResult
         */
        fun fromThrowable(t: Throwable): AppResult<Nothing> {
            val error = when {
                t is UnsatisfiedLinkError -> AppError.NativeLibraryNotAvailable()
                t.message?.contains("permission", ignoreCase = true) == true ->
                    AppError.PermissionDenied(t.message ?: "unknown")
                t.message?.contains("network", ignoreCase = true) == true ->
                    AppError.NetworkError(t.message ?: "network error", t)
                else -> AppError.Unknown(t.message ?: "unknown error", t)
            }
            return Failure(error)
        }
    }
}

/**
 * 重试策略
 */
object RetryStrategy {

    /**
     * 带重试的执行
     * @param maxRetries 最大重试次数
     * @param block 要执行的块
     * @return AppResult
     */
    suspend inline fun <T> withRetry(
        maxRetries: Int = 3,
        crossinline block: suspend () -> AppResult<T>
    ): AppResult<T> {
        var lastResult: AppResult<T>
        var attempt = 0
        do {
            lastResult = block()
            if (lastResult.isSuccess) return lastResult
            val error = (lastResult as AppResult.Failure).error
            if (!error.isRetryable || attempt >= maxRetries) return lastResult
            kotlinx.coroutines.delay(error.retryDelayMs)
            attempt++
        } while (attempt <= maxRetries)
        return lastResult
    }
}
