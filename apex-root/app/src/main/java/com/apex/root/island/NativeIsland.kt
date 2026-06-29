package com.apex.root.island

import com.apex.root.core.NativeLibraryLoader

object NativeIsland {
    fun createIsolatedEnv(name: String): Int = NativeLibraryLoader.safeCall(-1) {
        createIsolatedEnvNative(name)
    }

    fun destroyIsolatedEnv(pid: Int): Boolean = NativeLibraryLoader.safeCall(false) {
        destroyIsolatedEnvNative(pid)
    }

    private external fun createIsolatedEnvNative(name: String): Int
    private external fun destroyIsolatedEnvNative(pid: Int): Boolean
}
