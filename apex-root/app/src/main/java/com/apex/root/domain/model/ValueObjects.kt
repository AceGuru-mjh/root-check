package com.apex.root.domain.model

/**
 * 风险分值对象 — 封装验证逻辑
 */
@JvmInline
value class RiskScore private constructor(val value: Int) {
    init {
        require(value in 0..100) { "RiskScore must be 0-100, got $value" }
    }

    companion object {
        fun from(value: Int): Result<RiskScore> = runCatching {
            RiskScore(value.coerceIn(0, 100))
        }

        val SAFE = RiskScore(0)
        val MAX = RiskScore(100)
    }

    val level: RiskLevel
        get() = when {
            value > 60 -> RiskLevel.HIGH
            value > 30 -> RiskLevel.MEDIUM
            value > 10 -> RiskLevel.LOW
            else -> RiskLevel.SAFE
        }

    val label: String
        get() = when (level) {
            RiskLevel.SAFE -> "安全"
            RiskLevel.LOW -> "轻度风险"
            RiskLevel.MEDIUM -> "中等风险"
            RiskLevel.HIGH -> "高风险"
        }
}

enum class RiskLevel(val weight: Int) {
    SAFE(0), LOW(1), MEDIUM(2), HIGH(3);

    val isDangerous: Boolean get() = this == HIGH
    val isSafe: Boolean get() = this == SAFE
}

/**
 * 包名值对象
 */
@JvmInline
value class PackageName private constructor(val value: String) {
    init {
        require(value.matches(Regex("^[a-zA-Z][a-zA-Z0-9_]*(\\.[a-zA-Z][a-zA-Z0-9_]*)+$"))) {
            "Invalid package name: $value"
        }
    }

    companion object {
        fun from(value: String): Result<PackageName> = runCatching {
            PackageName(value.trim())
        }

        fun fromOrNull(value: String): PackageName? = runCatching { from(value).getOrThrow() }.getOrNull()
    }
}

/**
 * 扫描级别值对象
 */
@JvmInline
value class ScanDepth private constructor(val value: Int) {
    init {
        require(value in 0..3) { "ScanDepth must be 0-3, got $value" }
    }

    companion object {
        val QUICK = ScanDepth(0)
        val STANDARD = ScanDepth(1)
        val DEEP = ScanDepth(2)
        val FORENSIC = ScanDepth(3)

        fun from(value: Int): Result<ScanDepth> = runCatching {
            ScanDepth(value.coerceIn(0, 3))
        }
    }

    val label: String
        get() = when (value) {
            0 -> "快速检测"
            1 -> "标准检测"
            2 -> "深度检测"
            3 -> "取证检测"
            else -> "未知"
        }

    val estimatedDurationMs: Long
        get() = when (value) {
            0 -> 500L
            1 -> 3000L
            2 -> 15000L
            3 -> 60000L
            else -> 500L
        }
}

/**
 * 领域服务 — 集中业务规则
 */
object RiskDomainService {

    /**
     * 计算综合风险分（跨层加权）
     */
    fun calculateRiskScore(
        layerResults: Map<String, Boolean>,
        confidenceScores: Map<String, Float> = emptyMap()
    ): RiskScore {
        if (layerResults.isEmpty()) return RiskScore.SAFE

        val detectedLayers = layerResults.count { it.value }
        val totalLayers = layerResults.size
        val detectionRatio = detectedLayers.toFloat() / totalLayers

        // 跨层指数加权：跨多个检测层的告警权重指数级高
        val crossLayerBoost = if (detectedLayers > 3) 1.5f else 1.0f

        // 确定性加权：高置信度的检测结果权重更高
        val avgConfidence = if (confidenceScores.isNotEmpty()) {
            confidenceScores.values.filter { it > 0 }.average().toFloat()
        } else 0.8f

        val rawScore = (detectionRatio * 100 * crossLayerBoost * avgConfidence).toInt()
        return RiskScore.from(rawScore).getOrDefault(RiskScore.SAFE)
    }

    /**
     * 判断是否需要紧急处理
     */
    fun needsImmediateAction(score: RiskScore): Boolean = score.value > 60

    /**
     * 获取建议的治愈级别
     */
    fun recommendedCureLevel(score: RiskScore): CureLevel = when {
        score.value > 80 -> CureLevel.FACTORY
        score.value > 60 -> CureLevel.DEEP
        score.value > 30 -> CureLevel.STANDARD
        else -> CureLevel.LIGHT
    }
}
