package com.apex.root.domain.trust.model

data class SecurityAlert(
    val alertId: String,
    val type: AlertType,
    val severity: Severity,
    val description: String,
    val sourceReplica: String,
    val timestamp: Long,
    val evidence: ByteArray
)

enum class AlertType {
    PROCESS_TAMPER,
    MEMORY_MODIFIED,
    DEBUGGER_ATTACHED,
    PTRACE_DETECTED,
    REPLICA_FAILURE,
    IPC_TAMPER,
    SIGNATURE_MISMATCH
}
