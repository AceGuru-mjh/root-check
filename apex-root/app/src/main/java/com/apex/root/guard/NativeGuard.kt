package com.apex.root.guard

object NativeGuard {
    init {
        System.loadLibrary("apex_root")
    }

    external fun startGuardian(): Boolean
    external fun checkIntegrity(): Boolean
    external fun preventTamper(): Boolean
}
