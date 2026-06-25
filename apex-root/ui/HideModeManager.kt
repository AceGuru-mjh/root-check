package com.apex.root.island

import android.content.Context
import com.apex.root.data.jni.NativeBridge

class HideModeManager(private val ctx: Context) {

    companion object {
        const val MODE_DETECT = 0
        const val MODE_HIDE = 1
        const val MODE_GAME = 2
    }

    private val appUid: Int
        get() = ctx.applicationInfo.uid

    fun startHideMode(): Boolean {
        return NativeBridge.enableHideMode(appUid) == 0
    }

    fun startGameMode(): Boolean {
        return NativeBridge.enableGameMode(appUid) == 0
    }

    fun stopHideMode() {
        NativeBridge.disableHideMode()
    }

    fun isActive(): Boolean {
        return NativeBridge.isHideModeActive()
    }

    fun currentMode(): Int {
        return NativeBridge.getCurrentMode()
    }

    fun switchToMode(mode: Int): Boolean {
        return when (mode) {
            MODE_DETECT -> { stopHideMode(); !isActive() }
            MODE_HIDE -> startHideMode()
            MODE_GAME -> startGameMode()
            else -> false
        }
    }
}
