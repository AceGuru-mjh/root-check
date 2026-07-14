package com.apex.root.domain.guard.model

data class GuardState(
    val enabled: Boolean,
    val systemIntegrityOk: Boolean,
    val alertCount: Int
)
