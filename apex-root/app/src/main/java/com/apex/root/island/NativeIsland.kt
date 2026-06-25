package com.apex.root.island

object NativeIsland {
    init {
        System.loadLibrary("apex_root")
    }

    external fun createIsolatedEnv(name: String): Int
    external fun destroyIsolatedEnv(pid: Int): Boolean
}
