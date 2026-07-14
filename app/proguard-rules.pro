# Add project specific ProGuard rules here.
# By default, the flags in this file are appended to flags specified
# in /sdk/tools/proguard/proguard-android.txt

# Keep Compose
-keep class androidx.compose.** { *; }
-keepclassmembers class androidx.compose.** { *; }

# Keep Kotlin Coroutines
-keepnames class kotlinx.coroutines.internal.MainDispatcherFactory {}
-keepnames class kotlinx.coroutines.CoroutineExceptionHandler {}

# Keep Room
-keep class * extends androidx.room.RoomDatabase
-keep @androidx.room.Entity class *
-dontwarn androidx.room.paging.**

# Keep DataStore
-keep class com.apex.root.data.** { *; }

# Native methods
-keepclasseswithmembernames class * {
    native <methods>;
}

# Keep model classes
-keep class com.apex.root.domain.model.** { *; }
-keep class com.apex.root.data.protocol.** { *; }
