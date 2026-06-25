# ==================== 基础混淆配置 ====================
# 启用代码混淆和优化
-optimizationpasses 5
-optimizations !code/simplification/arithmetic,!code/allocation/variable,!field/*,!class/merger/*
-allowaccessmodification

# 保持行号信息用于调试（生产环境可移除）
-keepattributes SourceFile,LineNumberTable

# ==================== JNI 接口保护 ====================
# 保留所有 native 方法，防止 JNI 调用失败
-keepclasseswithmembernames class * {
    native <methods>;
}

# 保留 JNI 接口类（被 native 代码调用的类）
-keep class com.rootdetector.detect.DetectionResult { *; }
-keep class com.rootdetector.detect.DetectionMode { *; }
-keep class com.rootdetector.model.** { *; }

# ==================== 检测引擎核心类 ====================
# 保留检测引擎核心逻辑
-keep class com.rootdetector.detect.DetectionEngine { *; }
-keep class com.rootdetector.detector.** { *; }

# ==================== Gson 序列化保护 ====================
# 保留使用 Gson 序列化的数据类
-keep class com.rootdetector.model.DetectionReport { *; }
-keep class com.rootdetector.model.ReportSummary { *; }
-keep class com.rootdetector.model.ThreatLevel { *; }

# ==================== UI 和工具类 ====================
# 保留 Activity 和 ViewBinding
-keep class com.rootdetector.ui.** { *; }
-keep class com.rootdetector.databinding.** { *; }
-keep class * extends androidx.appcompat.app.AppCompatActivity

# 保留工具类
-keep class com.rootdetector.util.** { *; }

# ==================== 加壳保护类 ====================
# 保留加壳保护相关类（需要反射访问）
-keep class com.rootdetector.util.ApkProtector { *; }
-keep class com.rootdetector.util.ApkProtector$* { *; }

# ==================== 第三方库 ====================
# Gson
-keepattributes Signature
-keepattributes *Annotation*
-keep class sun.misc.Unsafe { *; }
-keep class com.google.gson.** { *; }

# AndroidX
-keep class androidx.** { *; }
-keep interface androidx.** { *; }

# ==================== 混淆策略 ====================
# 重命名策略
-repackageclasses 'a.b.c'
-flattenpackagehierarchy 'x.y.z'

# 优化策略
-keepclassmembers class * {
    @androidx.annotation.Keep *;
}

# 保留枚举
-keepclassmembers enum * {
    public static **[] values();
    public static ** valueOf(java.lang.String);
}

# 保留 Parcelable
-keep class * implements android.os.Parcelable {
    public static final ** CREATOR;
}

# 保留 Serializable
-keepclassmembers class * implements java.io.Serializable {
    static final long serialVersionUID;
    private static final java.io.ObjectStreamField[] serialPersistentFields;
    private void writeObject(java.io.ObjectOutputStream);
    private void readObject(java.io.ObjectInputStream);
    java.lang.Object writeReplace(java.lang.Object);
    java.lang.Object readResolve(java.lang.Object);
}

# ==================== 移除调试信息 ====================
# 移除日志调用（生产环境）
-assumenosideeffects class android.util.Log {
    public static boolean isLoggable(java.lang.String, int);
    public static int v(...);
    public static int i(...);
    public static int w(...);
    public static int d(...);
    public static int e(...);
}

# ==================== 防止反射破坏 ====================
-keepclassmembers class * {
    @android.webkit.JavascriptInterface <methods>;
}

# ==================== 保留资源引用 ====================
-keepclassmembers class **.R$* {
    public static <fields>;
}

# ==================== 检测相关特殊保护 ====================
# 保留所有检测器类的公共方法
-keepclassmembers class com.rootdetector.detector.** {
    public <methods>;
}

# 保留检测结果构造
-keepclassmembers class com.rootdetector.detect.DetectionResult {
    public <init>(...);
}
