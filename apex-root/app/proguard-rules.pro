# ═══════════════════════════════════════════════════════════
# APEX-Root ProGuard / R8 rules
# ----------------------------------------------------------------
# 全部 native 方法所在的类必须 keep — 否则 R8 重命名后, JNI
# 符号 (Java_com_apex_root_...) 找不到对应的 Java 方法。
# 同时保留所有 native 方法签名 (参数类型不能被混淆)。
# ═══════════════════════════════════════════════════════════

# ─── JNI bridge classes — keep class name + native method signatures ───
-keep class com.apex.root.data.jni.NativeBridge {
    native <methods>;
}
-keep class com.apex.root.data.jni.NativeBridge$* { *; }

-keep class com.apex.root.island.NativeIsland {
    native <methods>;
}
-keep class com.apex.root.cure.NativeCure {
    native <methods>;
}
-keep class com.apex.root.guard.NativeGuard {
    native <methods>;
}
-keep class com.apex.root.game.NativeGameMode {
    native <methods>;
}
-keep class com.apex.root.hid.NativeHwid {
    native <methods>;
}

# JNI also requires the *declaring class* of native methods to retain its
# exact name (the JNI symbol is mangled from the FQN). The
# -keepclasseswithmembernames above only keeps the member name, not the
# class name; for native methods we need both. The rules above are correct.
-keepclasseswithmembernames class * {
    native <methods>;
}

# ─── JNI-referenced model classes (used in native signatures / reflection) ───
-keep class com.apex.root.domain.model.** { *; }
-keep class com.apex.root.domain.guard.model.** { *; }
-keep class com.apex.root.domain.trust.model.** { *; }
-keep class com.apex.root.core.error.AppError { *; }

# ─── Protobuf generated classes (reflection-heavy) ───
-keep class com.apex.root.ipc.** { *; }
-keepclassmembers class * extends com.google.protobuf.GeneratedMessageLite { *; }
-dontwarn com.google.protobuf.**

# ─── Compose lifecycle / ViewModel (instantiated by reflection) ───
-keep class com.apex.root.viewmodel.** { *; }
-keep class com.apex.root.ApexRootApp { *; }
-keep class com.apex.root.ui.MainActivity { *; }

# ─── WorkManager — Worker classes instantiated by reflection ───
-keep class com.apex.root.work.** { *; }
-keep class androidx.work.** { *; }
-dontwarn androidx.work.**

# ─── NotificationCompat (Notifier) ───
-keep class com.apex.root.core.notification.** { *; }

# ─── Kotlinx Serialization (used by some libs) ───
-dontwarn kotlinx.serialization.**

# ─── Kotlin metadata — keep @Metadata annotation so reflection works ───
-keepattributes RuntimeVisibleAnnotations,RuntimeVisibleParameterAnnotations,RuntimeVisibleTypeAnnotations,Signature,InnerClasses,EnclosingMethod,*Annotation*

# ─── Remove debug logs in release builds ───
-assumenosideeffects class android.util.Log {
    public static *** v(...);
    public static *** d(...);
    public static *** i(...);
}
# Keep __android_log_print calls (native logs) — they are not removed by R8.
# This only strips Java-level Log.v/d/i calls.

# ─── Optimization: inline trivial methods, allow R8 full mode ───
-allowaccessmodification
# v1.1.3: 移除 -overloadaggressively (R8 已废弃,会产生警告)
-repackageclasses ''
-mergeinterfacesaggressively

# ─── Kotlin coroutines internals ───
# v1.1.3: 收窄 keep 规则,只保留必要部分 (原 -keep class kotlinx.coroutines.** { *; } 过宽)
-dontwarn kotlinx.coroutines.**
-keepclassmembers class kotlinx.coroutines.** {
    public static ** INSTANCE;
}
