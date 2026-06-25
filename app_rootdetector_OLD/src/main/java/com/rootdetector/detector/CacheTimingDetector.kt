package com.rootdetector.detector

import com.rootdetector.detect.DetectionResult
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

class CacheTimingDetector {

    private external fun nativeDetectCacheTiming(): DetectionResult

    companion object {
        init {
            System.loadLibrary("rootdetector")
        }
    }

    suspend fun detect(): DetectionResult = withContext(Dispatchers.IO) {
        nativeDetectCacheTiming()
    }
}
