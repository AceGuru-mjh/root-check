package com.apex.root.game

object NativeGameMode {
    init {
        System.loadLibrary("apex_root")
    }

    external fun enterGameMode(): Boolean
    external fun exitGameMode(): Boolean
    external fun isInGameMode(): Boolean
}
