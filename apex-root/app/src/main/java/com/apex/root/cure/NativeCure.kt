package com.apex.root.cure

object NativeCure {
    init {
        System.loadLibrary("apex_root")
    }

    external fun detectRootType(): Int
    external fun lightCleanup(): Boolean
    external fun standardFix(rootType: Int): Boolean
    external fun deepRecovery(): Boolean
    external fun factoryReset(): Boolean
}
