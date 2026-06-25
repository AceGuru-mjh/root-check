package com.rootdetector.ui

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.rootdetector.detect.DetectionEngine
import com.rootdetector.domain.model.DetectionLevel
import com.rootdetector.domain.model.ThreatLevel
import com.rootdetector.ui.model.DetectionUiState
import com.rootdetector.ui.model.HistoryEntry
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class MainViewModel(application: Application) : AndroidViewModel(application) {

    private val engine = DetectionEngine()

    private val _uiState = MutableStateFlow<DetectionUiState>(DetectionUiState.Idle)
    val uiState: StateFlow<DetectionUiState> = _uiState.asStateFlow()

    private val _history = MutableStateFlow<List<HistoryEntry>>(emptyList())
    val history: StateFlow<List<HistoryEntry>> = _history.asStateFlow()

    private val _selfCheckIssues = MutableStateFlow<List<String>>(emptyList())
    val selfCheckIssues: StateFlow<List<String>> = _selfCheckIssues.asStateFlow()

    fun runDetection(level: DetectionLevel) {
        if (_uiState.value is DetectionUiState.Running) return
        _uiState.value = DetectionUiState.Running
        viewModelScope.launch {
            val start = System.currentTimeMillis()
            try {
                val result = withContext(Dispatchers.IO) {
                    val base = engine.detect(level)
                    if (level == DetectionLevel.DEEP || level == DetectionLevel.FORENSIC) {
                        engine.detectWithAntiHiding(level)
                    } else {
                        base
                    }
                }
                val elapsed = System.currentTimeMillis() - start
                val threat = ThreatLevel.fromScore(result.riskScore)
                _uiState.value = DetectionUiState.Success(
                    level = level,
                    layerResults = result.layerResults,
                    rootDetected = result.rootDetected,
                    riskScore = result.riskScore,
                    threatLevel = threat,
                    elapsedMs = elapsed
                )
                appendHistory(result.reportId, level, result.rootDetected, result.riskScore, result.layerResults.size)
            } catch (e: Exception) {
                _uiState.value = DetectionUiState.Error(e.message ?: "Unknown error")
            }
        }
    }

    fun quickCheck() {
        if (_uiState.value is DetectionUiState.Running) return
        _uiState.value = DetectionUiState.Running
        viewModelScope.launch {
            val start = System.currentTimeMillis()
            try {
                val result = withContext(Dispatchers.IO) {
                    engine.quickCheck()
                }
                val elapsed = System.currentTimeMillis() - start
                val threat = ThreatLevel.fromScore(result.riskScore)
                _uiState.value = DetectionUiState.Success(
                    level = DetectionLevel.QUICK,
                    layerResults = result.layerResults,
                    rootDetected = result.rootDetected,
                    riskScore = result.riskScore,
                    threatLevel = threat,
                    elapsedMs = elapsed
                )
                appendHistory(result.reportId, DetectionLevel.QUICK, result.rootDetected, result.riskScore, result.layerResults.size)
            } catch (e: Exception) {
                _uiState.value = DetectionUiState.Error(e.message ?: "Unknown error")
            }
        }
    }

    fun runSelfCheck() {
        viewModelScope.launch(Dispatchers.IO) {
            val issues = engine.selfCheck()
            _selfCheckIssues.value = issues
        }
    }

    fun resetState() {
        _uiState.value = DetectionUiState.Idle
    }

    private fun appendHistory(id: String, level: DetectionLevel, detected: Boolean, risk: Int, count: Int) {
        val entry = HistoryEntry(
            id = id,
            timestamp = System.currentTimeMillis(),
            level = level,
            rootDetected = detected,
            riskScore = risk,
            layerCount = count
        )
        _history.value = listOf(entry) + _history.value.take(49)
    }

    override fun onCleared() {
        super.onCleared()
    }
}
