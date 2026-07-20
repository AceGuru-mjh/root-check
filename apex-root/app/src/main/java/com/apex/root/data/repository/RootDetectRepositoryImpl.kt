package com.apex.root.data.repository

import com.apex.root.data.DetectionCache
import com.apex.root.data.PackageDetector
import com.apex.root.data.jni.NativeBridge
import com.apex.root.domain.model.CureLevel
import com.apex.root.domain.model.CureResult
import com.apex.root.domain.model.GameModeState
import com.apex.root.domain.model.RiskLevel
import com.apex.root.domain.model.RootType
import com.apex.root.domain.model.ScanResult
import com.apex.root.game.NativeGameMode
import com.apex.root.cure.NativeCure

class RootDetectRepositoryImpl : com.apex.root.domain.repository.IRootDetectRepository {

    override fun runQuickScan(force: Boolean): ScanResult {
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

        var details = NativeBridge.runQuickScan()
        var isRooted = NativeBridge.isDeviceRooted()
        var riskScore = NativeBridge.getRiskScore()

        // FIX-D1 (P0): 补充 PackageManager 检测。
        // native 层的 L8/L9/L10/L23 路径检测在普通 app 上下文 (无 root + SELinux) 下
        // 100% 返回 EACCES,导致检测失效。这里通过 PackageManager.getInstalledPackages
        // 检测已安装的 root 框架 manager app (Magisk/KernelSU/APatch/Shizuku/Xposed),
        // 是对 native 路径检测的可靠补充。
        val pkgResult = runCatching { PackageDetector.detect() }
            .getOrNull()
            ?: PackageDetector.DetectionResult(
                magisk = false, kernelsu = false, apatch = false,
                shizuku = false, xposed = false, detectedPackages = emptyList()
            )

        if (pkgResult.anyDetected) {
            // 任一框架命中即视为已 root (PackageManager 检测可靠,优先级高于 native 路径检测)
            isRooted = true
            // 风险分至少提升到 DANGER 区间 (50) 以反映真实状态
            if (riskScore < 50) riskScore = 50
            val section = buildPackageManagerSection(pkgResult)
            details = if (details.isEmpty()) section else "$details\n$section"
        }

        // 修复：native 库未加载时 details 为空字符串，原代码会缓存空结果，
        // 之后 5 秒内重复扫描都返回缓存的空字符串，用户看到的是"风险分: 0"假象。
        // 现在仅在 details 非空时缓存，避免错误结果被持久化。
        if (details.isNotEmpty()) {
            DetectionCache.putString(DetectionCache.KEY_QUICK_SCAN, details)
            DetectionCache.putInt(DetectionCache.KEY_RISK_SCORE, riskScore)
            DetectionCache.putBoolean(DetectionCache.KEY_IS_ROOTED, isRooted)
        }

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

    /**
     * FIX-D1: 把 PackageManager 检测命中结果格式化为追加到 details 末尾的段落。
     * 仅在检测到至少一个 root 框架时才会被调用。
     */
    private fun buildPackageManagerSection(
        result: PackageDetector.DetectionResult
    ): String {
        val sb = StringBuilder()
        sb.appendLine()
        sb.appendLine("=== PackageManager 检测 (补充 native 路径检测在 SELinux 下失效) ===")
        sb.appendLine("检测到已安装 root/特权框架: ${result.detectedPackages.joinToString(", ")}")
        if (result.magisk) sb.appendLine("  - L8  Magisk: 命中")
        if (result.kernelsu) sb.appendLine("  - L9  KernelSU/SukiSU: 命中")
        if (result.apatch) sb.appendLine("  - L10 APatch: 命中")
        if (result.xposed) sb.appendLine("  - L11 Xposed/LSPosed: 命中")
        if (result.shizuku) sb.appendLine("  - L23 Shizuku/Dhizuku: 命中")
        return sb.toString().trimEnd()
    }

    override fun detectRootType(): RootType = RootType.fromValue(NativeCure.detectRootType())

    override fun applyCure(level: CureLevel): CureResult {
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

    override fun getGameModeState(): GameModeState {
        return GameModeState(
            active = NativeGameMode.isInGameMode(),
            hiddenProcesses = if (NativeGameMode.isInGameMode()) 5 else 0
        )
    }

    override fun toggleGameMode(): Boolean {
        return if (NativeGameMode.isInGameMode()) {
            NativeGameMode.exitGameMode()
        } else {
            NativeGameMode.enterGameMode()
        }
    }

    // ─── Enhanced detection methods ─────────────────────

    override fun runDeepDetection(): String {
        val sb = StringBuilder()
        sb.appendLine("=== APEX 深度检测报告 (Ring3 root 级) ===")
        sb.appendLine()
        sb.appendLine("[说明] 所有 Ring0 内核态检测已完全移除,仅保留 root 级 / 用户态 (Ring3) 检测")
        sb.appendLine("[说明] syscall 篡改检测改用 layer5_sidechannel.cpp 的侧信道时序分析")
        sb.appendLine()

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
        // 已移除：NativeBridge.detectSyscallTableHook()
        // 改用 syscall 结果一致性检测
        if (NativeBridge.detectSyscallResultInconsistency()) sb.appendLine("Syscall 结果不一致: 检测到（可能存在 hook）")

        // 5. 新增 L14 - 虚拟框架
        sb.appendLine()
        sb.appendLine("--- L14 虚拟框架 / 双开分身 ---")
        sb.appendLine(NativeBridge.virtualXposedFullScan())

        // 6. 新增 L15 - 危险应用
        sb.appendLine()
        sb.appendLine("--- L15 危险应用 ---")
        sb.appendLine(NativeBridge.dangerousAppsFullScan())

        // 7. 新增 L16 - Magisk 扩展
        sb.appendLine()
        sb.appendLine("--- L16 Magisk 扩展 ---")
        sb.appendLine(NativeBridge.magiskExtensionsFullScan())

        // 8. 新增独立隐藏框架检测
        sb.appendLine()
        sb.appendLine("--- 隐藏框架 ---")
        if (NativeBridge.detectHideMyApplist()) sb.appendLine("HideMyApplist: 检测到")
        if (NativeBridge.detectStorageIsolation()) sb.appendLine("StorageIsolation: 检测到")
        if (NativeBridge.detectMagiskHideLegacy()) sb.appendLine("MagiskHide (legacy): 检测到")
        if (NativeBridge.detectMagiskDenyList()) sb.appendLine("Magisk DenyList: 检测到")

        // 9. v1.1.0 新增 L17-L20 检测层
        sb.appendLine()
        sb.appendLine("--- L17 现代 Root Fork (v1.1.0) ---")
        sb.appendLine(NativeBridge.modernRootForksFullScan())

        sb.appendLine()
        sb.appendLine("--- L18 APatch KPM / KernelPatch (v1.1.0) ---")
        sb.appendLine(NativeBridge.apatchKpmFullScan())

        sb.appendLine()
        sb.appendLine("--- L19 隐藏框架深度检测 (v1.1.0) ---")
        sb.appendLine(NativeBridge.hideFrameworksFullScan())

        sb.appendLine()
        sb.appendLine("--- L20 现代 Hook 框架 (v1.1.0) ---")
        sb.appendLine(NativeBridge.modernHooksFullScan())

        return sb.toString()
    }

    override fun getMemoryFingerprintMask(): Int {
        var mask = DetectionCache.getInt(DetectionCache.KEY_MEM_FINGERPRINT)
        if (mask == null) {
            mask = NativeBridge.fullMemoryFingerprint()
            DetectionCache.putInt(DetectionCache.KEY_MEM_FINGERPRINT, mask)
        }
        return mask
    }

    override fun hasShamiko(): Boolean {
        var result = DetectionCache.getBoolean(DetectionCache.KEY_SHAMIKO)
        if (result == null) {
            result = NativeBridge.detectShamiko()
            DetectionCache.putBoolean(DetectionCache.KEY_SHAMIKO, result)
        }
        return result
    }

    override fun hasZygiskNext(): Boolean {
        var result = DetectionCache.getBoolean(DetectionCache.KEY_ZYGISK_NEXT)
        if (result == null) {
            result = NativeBridge.detectZygiskNext()
            DetectionCache.putBoolean(DetectionCache.KEY_ZYGISK_NEXT, result)
        }
        return result
    }
}
