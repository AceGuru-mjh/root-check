package com.rootdetector.domain.usecase

import com.rootdetector.domain.model.DetectionLevel
import com.rootdetector.domain.model.DetectionResult
import com.rootdetector.domain.repository.DetectionRepository

class RunDetectionUseCase(
    private val repository: DetectionRepository
) {
    suspend operator fun invoke(level: DetectionLevel): DetectionResult {
        val result = repository.detect(level)
        return result
    }

    suspend fun quick(): DetectionResult {
        return repository.quickDetect()
    }
}