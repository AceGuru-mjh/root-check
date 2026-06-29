package com.apex.root.cure

import com.apex.root.core.NativeLibraryLoader

object NativeCure {
    fun detectRootType(): Int = NativeLibraryLoader.safeCall(-1) {
        detectRootTypeNative()
    }

    fun lightCleanup(): Boolean = NativeLibraryLoader.safeCall(false) {
        lightCleanupNative()
    }

    fun standardFix(rootType: Int): Boolean = NativeLibraryLoader.safeCall(false) {
        standardFixNative(rootType)
    }

    fun deepRecovery(): Boolean = NativeLibraryLoader.safeCall(false) {
        deepRecoveryNative()
    }

    fun factoryReset(): Boolean = NativeLibraryLoader.safeCall(false) {
        factoryResetNative()
    }

    private external fun detectRootTypeNative(): Int
    private external fun lightCleanupNative(): Boolean
    private external fun standardFixNative(rootType: Int): Boolean
    private external fun deepRecoveryNative(): Boolean
    private external fun factoryResetNative(): Boolean
}
