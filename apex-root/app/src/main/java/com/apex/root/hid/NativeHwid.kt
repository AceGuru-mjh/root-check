package com.apex.root.hid

import com.apex.root.core.NativeLibraryLoader

object NativeHwid {
    fun spoofAll(): Boolean = NativeLibraryLoader.safeCall(false) {
        spoofAllNative()
    }

    fun restoreReal(): Boolean = NativeLibraryLoader.safeCall(false) {
        restoreRealNative()
    }

    private external fun spoofAllNative(): Boolean
    private external fun restoreRealNative(): Boolean
}
