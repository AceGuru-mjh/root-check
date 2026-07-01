package com.apex.root.domain.repository

import com.apex.root.domain.model.ScanResult
import com.apex.root.domain.model.CureResult
import com.apex.root.domain.model.CureLevel
import com.apex.root.domain.model.RootType
import com.apex.root.domain.model.GameModeState

/**
 * Root 检测 Repository 接口
 * 定义所有检测相关的业务操作契约
 */
interface IRootDetectRepository {

    /** 快速扫描 */
    fun runQuickScan(force: Boolean = false): ScanResult

    /** 深度扫描 */
    fun runDeepDetection(): String

    /** 检测 root 类型 */
    fun detectRootType(): RootType

    /** 获取内存指纹掩码 */
    fun getMemoryFingerprintMask(): Int

    /** 检测 Shamiko */
    fun hasShamiko(): Boolean

    /** 检测 ZygiskNext */
    fun hasZygiskNext(): Boolean

    /** 治愈 */
    fun applyCure(level: CureLevel): CureResult

    /** 游戏模式状态 */
    fun getGameModeState(): GameModeState

    /** 切换游戏模式 */
    fun toggleGameMode(): Boolean
}
