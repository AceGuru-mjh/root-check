package com.apex.root.game

import com.apex.root.core.NativeLibraryLoader

object NativeGameMode {
    fun enterGameMode(): Boolean = NativeLibraryLoader.safeCall(false) {
        enterGameModeNative()
    }

    fun exitGameMode(): Boolean = NativeLibraryLoader.safeCall(false) {
        exitGameModeNative()
    }

    fun isInGameMode(): Boolean = NativeLibraryLoader.safeCall(false) {
        isInGameModeNative()
    }

    private external fun enterGameModeNative(): Boolean
    private external fun exitGameModeNative(): Boolean
    private external fun isInGameModeNative(): Boolean
}
