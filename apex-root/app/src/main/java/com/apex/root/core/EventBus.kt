package com.apex.root.core

import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.receiveAsFlow

/**
 * 一次性事件总线
 * 用于处理 Toast、导航等一次性事件，避免配置变更时重复触发
 */
object EventBus {

    private val _events = Channel<UiEvent>(Channel.BUFFERED)
    val events: Flow<UiEvent> = _events.receiveAsFlow()

    suspend fun emit(event: UiEvent) {
        _events.send(event)
    }

    fun tryEmit(event: UiEvent) {
        _events.trySend(event)
    }
}

/**
 * UI 一次性事件
 */
sealed class UiEvent {
    data class ShowToast(val message: String, val type: ToastType = ToastType.INFO) : UiEvent()
    data class Navigate(val route: String) : UiEvent()
    data class ShowError(val error: com.apex.root.core.error.AppError) : UiEvent()
    data class ShowSnackbar(val message: String, val actionLabel: String? = null) : UiEvent()
    object NavigateBack : UiEvent()
}

enum class ToastType { INFO, SUCCESS, WARNING, ERROR }
