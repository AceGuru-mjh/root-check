package com.apex.root.domain.parallel

import com.apex.root.data.jni.NativeBridge

/**
 * 交叉验证 + 误报抑制 — v1.1.0 新增。
 *
 * 单层检测可能因以下原因误报:
 *  - L1 属性检测: 某些 OEM 自定义 ROM 设置 ro.debuggable=1 用于开发
 *  - L2 ART 注入: Compose / KSP 等会引入额外的 .oat 文件
 *  - L3 内存特征: 安卓系统的 JIT 缓存可能被误认为可疑
 *  - L11 Hook: 现代 Android Studio 的 Layout Inspector 会注入 hook
 *  - L14 虚拟框架: 工作配置文件 (work profile) 可能被误识为双开
 *
 * 本模块定义"验证规则":某些层的检测需要被另一层交叉确认才算可信。
 * 例如:
 *  - L1 单独检测到 → 视为可疑但低置信度 (可能误报)
 *  - L1 + L6 同时检测到 → 高置信度 (属性 + root daemon 双信号)
 *  - L2 单独检测到 → 低置信度 (可能是 Compose 引入的 .oat)
 *  - L2 + L3 同时检测到 → 高置信度 (ART 注入 + 内存特征双信号)
 */
object CrossValidator {

    /**
     * 验证规则:定义哪些层的组合才算高置信度检测。
     *
     * 每条规则的语义:
     *   如果 triggerLayers 中所有层都检测到,且 confirmLayers 中至少
     *   有一层检测到,则该规则"通过",对应 rootType 的置信度提升。
     */
    data class ValidationRule(
        val name: String,
        val rootType: String,
        val triggerLayers: Set<Int>,    // 触发层 (全部满足)
        val confirmLayers: Set<Int>,    // 确认层 (至少一个满足)
        val confidenceBoost: Int        // 置信度加成 (0-100)
    )

    private val rules = listOf(
        // Magisk 家族: L8 (Magisk daemon) + 任意一个 L3/L4/L6/L16
        ValidationRule(
            name = "Magisk 多重信号确认",
            rootType = "MAGISK",
            triggerLayers = setOf(8),
            confirmLayers = setOf(3, 4, 6, 16),
            confidenceBoost = 25
        ),
        // KernelSU 家族: L9 + L4 (mount) 或 L6 (daemon)
        ValidationRule(
            name = "KernelSU 多重信号确认",
            rootType = "KERNELSU",
            triggerLayers = setOf(9),
            confirmLayers = setOf(4, 6, 19),
            confidenceBoost = 25
        ),
        // APatch 家族: L10 + L18 (KPM 用户态)
        ValidationRule(
            name = "APatch + KPM 双重确认",
            rootType = "APATCH",
            triggerLayers = setOf(10),
            confirmLayers = setOf(18),
            confidenceBoost = 30
        ),
        // SukiSU: L17 + L6/L9 (root daemon / KSU 信号)
        ValidationRule(
            name = "SukiSU 多重信号确认",
            rootType = "SUKISU",
            triggerLayers = setOf(17),
            confirmLayers = setOf(6, 9),
            confidenceBoost = 25
        ),
        // Magisk Delta: L17 + L8 (Magisk daemon)
        ValidationRule(
            name = "Magisk Delta 双重确认",
            rootType = "MAGISK_DELTA",
            triggerLayers = setOf(17),
            confirmLayers = setOf(8),
            confidenceBoost = 25
        ),
        // Hook 框架确认: L11 + L2 (ART) 或 L20 (现代 hook)
        ValidationRule(
            name = "Hook 框架双重确认",
            rootType = "HOOK_FRAMEWORK",
            triggerLayers = setOf(11),
            confirmLayers = setOf(2, 20),
            confidenceBoost = 20
        ),
        // 隐藏框架: L19 + L16 (Magisk 扩展)
        ValidationRule(
            name = "隐藏框架双重确认",
            rootType = "HIDE_FRAMEWORK",
            triggerLayers = setOf(19),
            confirmLayers = setOf(16, 19),
            confidenceBoost = 20
        )
    )

    /**
     * 已知的误报场景 — 这些情况下的单层检测应被降权。
     */
    data class FalsePositiveRule(
        val name: String,
        val appliesToLayer: Int,
        val condition: () -> Boolean,
        val confidencePenalty: Int
    )

    private val falsePositiveRules = listOf(
        FalsePositiveRule(
            name = "Android Studio Layout Inspector 注入",
            appliesToLayer = 11,
            condition = {
                // 检测是否在 emulator 或 developer mode 下运行
                runCatching {
                    android.os.Build.FINGERPRINT.contains("generic", ignoreCase = true) ||
                    android.os.Build.MODEL.contains("emulator", ignoreCase = true) ||
                    android.os.Build.MODEL.contains("Android SDK", ignoreCase = true)
                }.getOrDefault(false)
            },
            confidencePenalty = 30
        ),
        FalsePositiveRule(
            name = "Compose 多 OAT 文件",
            appliesToLayer = 2,
            condition = {
                // Compose 应用通常有 30+ OAT 文件,但这不一定代表 hook
                runCatching {
                    val s = NativeBridge.runQuickScan()
                    s.contains("L2 ART注入:      ❌ 异常") &&
                    !s.contains("L3 内存特征:     ❌ 异常") &&
                    !s.contains("L11 Hook框架:    ❌ 异常")
                }.getOrDefault(false)
            },
            confidencePenalty = 25
        )
    )

    /**
     * 对一次扫描结果应用交叉验证规则,返回增强后的结果。
     *
     * @param baseResult 原始并行扫描结果
     * @return 包含置信度评分与 root type 推断的增强结果
     */
    fun validate(baseResult: ParallelScanResult): ValidationResult {
        val detectedLayerIds = baseResult.layers.filter { it.detected }.map { it.layerId }.toSet()

        // 应用验证规则
        val matchedRules = rules.filter { rule ->
            rule.triggerLayers.all { it in detectedLayerIds } &&
            rule.confirmLayers.any { it in detectedLayerIds }
        }

        // 应用误报规则
        val matchedFPRules = falsePositiveRules.filter { rule ->
            rule.appliesToLayer in detectedLayerIds && rule.condition()
        }

        // 计算置信度
        val baseConfidence = if (baseResult.detectedCount == 0) 0
            else minOf(100, baseResult.detectedCount * 15)
        val boost = matchedRules.sumOf { it.confidenceBoost }
        val penalty = matchedFPRules.sumOf { it.confidencePenalty }
        val finalConfidence = (baseConfidence + boost - penalty).coerceIn(0, 100)

        // 推断最可能的 root 类型
        val inferredRootType = matchedRules
            .groupBy { it.rootType }
            .maxByOrNull { (_, rules) -> rules.sumOf { it.confidenceBoost } }
            ?.key ?: "UNKNOWN"

        return ValidationResult(
            baseResult = baseResult,
            confidence = finalConfidence,
            inferredRootType = inferredRootType,
            matchedValidationRules = matchedRules.map { it.name },
            matchedFalsePositiveRules = matchedFPRules.map { it.name },
            isHighConfidence = finalConfidence >= 70,
            isLikelyFalsePositive = penalty >= 50
        )
    }
}

/**
 * 交叉验证结果。
 */
data class ValidationResult(
    val baseResult: ParallelScanResult,
    val confidence: Int,           // 0-100
    val inferredRootType: String,
    val matchedValidationRules: List<String>,
    val matchedFalsePositiveRules: List<String>,
    val isHighConfidence: Boolean,
    val isLikelyFalsePositive: Boolean
) {
    /**
     * 人类可读的结论文本 (用于 UI 显示)。
     */
    val conclusion: String get() = when {
        baseResult.detectedCount == 0 -> "✅ 设备安全 (0/${baseResult.totalLayers} 层检测到异常)"
        isLikelyFalsePositive -> "⚠️ 可能误报 (${baseResult.detectedCount} 层检测到,但匹配 ${matchedFalsePositiveRules.size} 条误报规则)"
        isHighConfidence -> "❌ 高置信度检测到 $inferredRootType (置信度 $confidence%)"
        else -> "⚠️ 低置信度检测 (${baseResult.detectedCount} 层, 置信度 $confidence%, 推断: $inferredRootType)"
    }
}
