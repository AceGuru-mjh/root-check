package com.rootdetector.detect

data class DetectionResult(
    val detected: Boolean,
    val layer: String,
    val detail: String
)
