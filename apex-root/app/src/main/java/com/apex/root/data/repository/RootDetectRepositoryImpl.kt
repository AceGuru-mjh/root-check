package com.apex.root.data.repository

import com.apex.root.data.DetectionCache
import com.apex.root.data.jni.NativeBridge
import com.apex.root.domain.model.CureLevel
import com.apex.root.domain.model.CureResult
import com.apex.root.domain.model.GameModeState
import com.apex.root.domain.model.RiskLevel
import com.apex.root.domain.model.RootType
import com.apex.root.domain.model.ScanResult
import com.apex.root.game.NativeGameMode
import com.apex.root.cure.NativeCure

class RootDetectRepositoryImpl {

    fun runQuickScan(force: Boolean = false): ScanResult {
        // Cache check for quick scan
        if (!force) {
            DetectionCache.getString(DetectionCache.KEY_QUICK_SCAN)?.let { details ->
                val riskScore = DetectionCache.getInt(DetectionCache.KEY_RISK_SCORE) ?: 0
                val isRooted = DetectionCache.getBoolean(DetectionCache.KEY_IS_ROOTED) ?: false
                val riskLevel = when {
                    riskScore > 60 -> RiskLevel.CRITICAL
                    riskScore > 30 -> RiskLevel.DANGER
                    riskScore > 10 -> RiskLevel.WARNING
                    else -> RiskLevel.SAFE
                }
                return ScanResult(
                    isRooted = isRooted,
                    riskLevel = riskLevel,
                    details = details,
                    riskScore = riskScore
                )
            }
        }

        val details = NativeBridge.runQuickScan()
        val isRooted = NativeBridge.isDeviceRooted()
        val riskScore = NativeBridge.getRiskScore()

        // Cache results
        DetectionCache.putString(DetectionCache.KEY_QUICK_SCAN, details)
        DetectionCache.putInt(DetectionCache.KEY_RISK_SCORE, riskScore)
        DetectionCache.putBoolean(DetectionCache.KEY_IS_ROOTED, isRooted)

        val riskLevel = when {
            riskScore > 60 -> RiskLevel.CRITICAL
            riskScore > 30 -> RiskLevel.DANGER
            riskScore > 10 -> RiskLevel.WARNING
            else -> RiskLevel.SAFE
        }

        return ScanResult(
            isRooted = isRooted,
            riskLevel = riskLevel,
            details = details,
            riskScore = riskScore
        )
    }

    fun detectRootType(): RootType = RootType.fromValue(NativeCure.detectRootType())

    fun applyCure(level: CureLevel): CureResult {
        val rootType = detectRootType()
        // Invalidate cache after cure
        DetectionCache.invalidateAll()
        val success = when (level) {
            CureLevel.LIGHT -> NativeCure.lightCleanup()
            CureLevel.STANDARD -> NativeCure.standardFix(rootType.value)
            CureLevel.DEEP -> NativeCure.deepRecovery()
            CureLevel.FACTORY -> NativeCure.factoryReset()
        }
        return CureResult(
            success = success,
            rootType = rootType,
            levelUsed = level,
            message = when (level) {
                CureLevel.LIGHT -> "轻度清理完成"
                CureLevel.STANDARD -> "标准修复完成，建议重启"
                CureLevel.DEEP -> "深度恢复完成，请重启设备"
                CureLevel.FACTORY -> "即将进入恢复模式"
            },
            needsReboot = level >= CureLevel.STANDARD
        )
    }

    fun getGameModeState(): GameModeState {
        return GameModeState(
            active = NativeGameMode.isInGameMode(),
            hiddenProcesses = if (NativeGameMode.isInGameMode()) 5 else 0
        )
    }

    fun toggleGameMode(): Boolean {
        return if (NativeGameMode.isInGameMode()) {
            NativeGameMode.exitGameMode()
        } else {
            NativeGameMode.enterGameMode()
        }
    }

    // ─── Enhanced detection methods ─────────────────────

    fun runDeepDetection(): String {
        val sb = StringBuilder()
        sb.appendLine("=== APEX 深度检测报告 ===")

        // 1. Memory fingerprint
        val memMask = NativeBridge.fullMemoryFingerprint()
        if (memMask != 0) {
            sb.appendLine("内存指纹: 发现可疑映射 ($memMask)")
        }
        sb.appendLine(NativeBridge.deepMemoryScanReport())

        // 2. SELinux context
        sb.appendLine(NativeBridge.selinuxFullScan())

        // 3. Anti-hiding probes
        sb.appendLine(NativeBridge.shamikoFullScan())
        sb.appendLine(NativeBridge.zygiskNextFullScan())

        // 4. Process/Mount hiding
        if (NativeBridge.detectProcessHiding()) sb.appendLine("进程隐藏: 检测到")
        if (NativeBridge.detectMountNamespaceHiding()) sb.appendLine("挂载命名空间隐藏: 检测到")
        if (NativeBridge.detectSyscallTableHook()) sb.appendLine("系统调用表Hook: 检测到")

        return sb.toString()
    }

    fun getMemoryFingerprintMask(): Int {
        var mask = DetectionCache.getInt(DetectionCache.KEY_MEM_FINGERPRINT)
        if (mask == null) {
            mask = NativeBridge.fullMemoryFingerprint()
            DetectionCache.putInt(DetectionCache.KEY_MEM_FINGERPRINT, mask)
        }
        return mask
    }

    fun hasShamiko(): Boolean {
        var result = DetectionCache.getBoolean(DetectionCache.KEY_SHAMIKO)
        if (result == null) {
            result = NativeBridge.detectShamiko()
            DetectionCache.putBoolean(DetectionCache.KEY_SHAMIKO, result)
        }
        return result
    }

    fun hasZygiskNext(): Boolean {
        var result = DetectionCache.getBoolean(DetectionCache.KEY_ZYGISK_NEXT)
        if (result == null) {
            result = NativeBridge.detectZygiskNext()
            DetectionCache.putBoolean(DetectionCache.KEY_ZYGISK_NEXT, result)
        }
        return result
    }
}
