package com.rootdetector.config

data class HideProfile(
    val name: String,
    val level: Int,
    val enabledItems: Set<String>,
    val isCustom: Boolean = false
) {
    companion object {
        fun preset(level: Int): HideProfile {
            val name = when (level) {
                1 -> "基础隐藏 (H0)"
                2 -> "标准隐藏 (H1)"
                3 -> "深度隐藏 (H2)"
                4 -> "取证级隐藏 (H3)"
                else -> "自定义"
            }
            return HideProfile(
                name = name,
                level = level,
                enabledItems = HideItem.defaultsForLevel(level)
            )
        }

        val PRESETS = listOf(
            preset(1), preset(2), preset(3), preset(4)
        )

        val PRESET_NAMES = listOf("基础 H0", "标准 H1", "深度 H2", "取证 H3")

        fun basic() = preset(1)
        fun standard() = preset(2)
        fun deep() = preset(3)
        fun forensic() = preset(4)

        fun custom(items: Set<String>): HideProfile {
            return HideProfile(
                name = "自定义",
                level = 0,
                enabledItems = items,
                isCustom = true
            )
        }
    }

    val totalItems: Int get() = HideItem.entries.size
    val enabledCount: Int get() = enabledItems.size
    val items: List<HideItem> get() = HideItem.entries.filter { it.id in enabledItems }
}

data class HideConfig(
    val profile: HideProfile,
    val autoApply: Boolean = false,
    val persistent: Boolean = false
)
