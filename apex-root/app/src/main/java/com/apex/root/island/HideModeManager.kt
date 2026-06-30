package com.apex.root.island

import android.content.Context
import com.apex.root.data.jni.NativeBridge

/**
 * 隐藏模式管理器 — 统一管理 Detection / Hide / Game 三种模式的切换。
 *
 * - [MODE_DETECT]：检测模式，不做任何隐藏，仅做设备完整性扫描
 * - [MODE_HIDE]：隐藏模式，对除 APEX-Root 外的所有应用隐藏 root 痕迹
 * - [MODE_GAME]：游戏模式，aggressive 隐藏 + 性能优化
 *
 * 原始版本位于仓库根 ui/HideModeManager.kt，但因不在 app/src/main/java/
 * 编译路径内而无法被 Gradle 编译进 APK。本文件已迁移到正确包路径。
 */
class HideModeManager(private val ctx: Context) {

    companion object {
        const val MODE_DETECT = 0
        const val MODE_HIDE = 1
        const val MODE_GAME = 2

        /**
         * 模式名称转换（用于 UI 显示与持久化）
         */
        fun modeName(mode: Int): String = when (mode) {
            MODE_DETECT -> "Detection"
            MODE_HIDE -> "Hide"
            MODE_GAME -> "Game"
            else -> "Unknown"
        }

        /**
         * 模式中文描述
         */
        fun modeDescription(mode: Int): String = when (mode) {
            MODE_DETECT -> "检测模式 · 仅扫描，不隐藏"
            MODE_HIDE -> "隐藏模式 · 对其他应用隐藏 root"
            MODE_GAME -> "游戏模式 · 激进隐藏 + 性能优化"
            else -> "未知模式"
        }
    }

    private val appUid: Int
        get() = ctx.applicationInfo.uid

    /**
     * 启动隐藏模式（对其他应用隐藏 root 痕迹）
     * @return true 表示成功切换到 Hide 模式
     */
    fun startHideMode(): Boolean {
        return NativeBridge.enableHideMode(appUid) == 0
    }

    /**
     * 启动游戏模式（aggressive 隐藏 + 性能优化）
     * @return true 表示成功切换到 Game 模式
     */
    fun startGameMode(): Boolean {
        return NativeBridge.enableGameMode(appUid) == 0
    }

    /**
     * 停止隐藏模式（回到 Detection 模式）
     */
    fun stopHideMode() {
        NativeBridge.disableHideMode()
    }

    /**
     * 隐藏模式是否激活（Hide 或 Game 任一）
     */
    fun isActive(): Boolean {
        return NativeBridge.isHideModeActive()
    }

    /**
     * 获取当前模式（0=Detect / 1=Hide / 2=Game）
     */
    fun currentMode(): Int {
        return NativeBridge.getCurrentMode()
    }

    /**
     * 获取最近一次错误信息（native 层）
     */
    fun lastError(): String {
        return NativeBridge.getLastError()
    }

    /**
     * 切换到指定模式
     * @param mode 目标模式（MODE_DETECT / MODE_HIDE / MODE_GAME）
     * @return true 表示切换成功
     */
    fun switchToMode(mode: Int): Boolean {
        return when (mode) {
            MODE_DETECT -> {
                stopHideMode()
                !isActive()
            }
            MODE_HIDE -> startHideMode()
            MODE_GAME -> startGameMode()
            else -> false
        }
    }
}
