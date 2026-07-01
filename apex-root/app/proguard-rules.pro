# Keep JNI native methods — 方法名必须与 C++ JNI 符号 (Java_..._<name>Native) 一致
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

# Keep model classes (used by JNI / serialization)
-keep class com.apex.root.domain.model.** { *; }
-keep class com.apex.root.domain.guard.model.** { *; }

# Keep protobuf generated classes
-keep class com.apex.root.ipc.** { *; }
-keepclassmembers class * extends com.google.protobuf.GeneratedMessageLite { *; }

# Keep Compose lifecycle / ViewModel
-keep class com.apex.root.viewmodel.** { *; }
-keep class com.apex.root.ApexRootApp { *; }
