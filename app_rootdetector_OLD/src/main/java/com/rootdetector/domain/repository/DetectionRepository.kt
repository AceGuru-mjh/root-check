package com.rootdetector.domain.repository

import com.rootdetector.domain.model.DetectionLevel
import com.rootdetector.domain.model.DetectionResult

interface DetectionRepository {
    suspend fun detect(level: DetectionLevel): DetectionResult
    suspend fun quickDetect(): DetectionResult
    fun isDaemonRunning(): Boolean
    suspend fun startDaemon(): Boolean
}