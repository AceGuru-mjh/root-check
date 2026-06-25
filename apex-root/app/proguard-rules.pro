# Keep JNI native methods
-keepclasseswithmembernames class com.apex.root.data.jni.NativeBridge {
    native <methods>;
}
-keepclasseswithmembernames class com.apex.root.island.NativeIsland {
    native <methods>;
}
-keepclasseswithmembernames class com.apex.root.cure.NativeCure {
    native <methods>;
}
-keepclasseswithmembernames class com.apex.root.guard.NativeGuard {
    native <methods>;
}
-keepclasseswithmembernames class com.apex.root.game.NativeGameMode {
    native <methods>;
}
-keepclasseswithmembernames class com.apex.root.hid.NativeHwid {
    native <methods>;
}

# Keep model classes
-keep class com.apex.root.domain.model.** { *; }
-keep class com.apex.root.domain.guard.model.** { *; }

# Keep service
-keep class com.apex.root.core.TrustedDaemonService { *; }
-keep class com.apex.root.receiver.AdminReceiver { *; }
