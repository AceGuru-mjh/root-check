package com.rootdetector.config

data class DetectionProfile(
    val name: String,
    val level: Int,
    val enabledItems: Set<String>,
    val isCustom: Boolean = false
) {
    companion object {
        fun preset(level: Int): DetectionProfile {
            val name = when (level) {
                1 -> "基础检测 (L0)"
                2 -> "标准检测 (L1)"
                3 -> "深度检测 (L2)"
                4 -> "取证检测 (L3)"
                else -> "自定义"
            }
            return DetectionProfile(
                name = name,
                level = level,
                enabledItems = DetectionItem.defaultsForLevel(level)
            )
        }

        val PRESETS = listOf(
            preset(1), preset(2), preset(3), preset(4)
        )

        val PRESET_NAMES = listOf("基础 L0", "标准 L1", "深度 L2", "取证 L3")

        fun quick() = preset(1)
        fun standard() = preset(2)
        fun deep() = preset(3)
        fun forensic() = preset(4)

        fun custom(items: Set<String>): DetectionProfile {
            return DetectionProfile(
                name = "自定义",
                level = 0,
                enabledItems = items,
                isCustom = true
            )
        }
    }

    val totalItems: Int get() = DetectionItem.entries.size
    val enabledCount: Int get() = enabledItems.size
    val items: List<DetectionItem> get() = DetectionItem.entries.filter { it.id in enabledItems }
}

data class DetectionConfig(
    val profile: DetectionProfile,
    val useDaemon: Boolean = true,
    val useCache: Boolean = false,
    val enableAntiHiding: Boolean = true,
    val timeoutMs: Long = 30000
)
