package com.rootdetector.domain.model

enum class DetectionLevel(val value: Int, val displayName: String) {
    QUICK(1, "快速检测"),
    STANDARD(2, "标准检测"),
    DEEP(3, "深度检测"),
    FORENSIC(4, "取证检测");

    companion object {
        fun fromValue(v: Int) = entries.firstOrNull { it.value == v } ?: STANDARD
    }
}