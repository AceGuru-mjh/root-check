package com.apex.root.guard

import com.apex.root.core.NativeLibraryLoader

object NativeGuard {
    fun startGuardian(): Boolean = NativeLibraryLoader.safeCall(false) {
        startGuardianNative()
    }

    fun checkIntegrity(): Boolean = NativeLibraryLoader.safeCall(false) {
        checkIntegrityNative()
    }

    fun preventTamper(): Boolean = NativeLibraryLoader.safeCall(false) {
        preventTamperNative()
    }

    private external fun startGuardianNative(): Boolean
    private external fun checkIntegrityNative(): Boolean
    private external fun preventTamperNative(): Boolean
}
