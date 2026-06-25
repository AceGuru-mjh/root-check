package com.apex.root.hid

object NativeHwid {
    init {
        System.loadLibrary("apex_root")
    }

    external fun spoofAll(): Boolean
    external fun restoreReal(): Boolean
}
