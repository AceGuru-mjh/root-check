package com.apex.root.domain.trust.model

data class GlobalSecureReport(
    val reportId: String,
    val timestamp: Long,
    val taskId: String,
    val results: List<TrustedLayerResult>,
    val consensusSignatures: List<ByteArray>,
    val overallRisk: Severity,
    val riskScore: Float,
    val daemonSignature: ByteArray
)
