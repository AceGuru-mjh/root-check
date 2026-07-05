# 开发指南

本文档介绍 APEX-Root 的开发规范、调试技巧和贡献流程。

---

## 🚀 快速开始

### 环境准备

请先完成 [环境搭建](./BUILD_ENV.md)。

### 首次构建

```bash
git clone https://github.com/mengjinghao/root-check.git
cd root-check/apex-root
echo "sdk.dir=$HOME/android-sdk" > local.properties
./gradlew assembleDebug
```

### 导入 Android Studio

1. 打开 Android Studio
2. File → Open → 选择 `apex-root/` 目录
3. 等待 Gradle Sync 完成
4. 选择 `app` 配置 → 点击 Run

---

## 📁 代码组织

### 目录结构

```
app/src/main/java/com/apex/root/
├── ui/
│   └── compose/
│       ├── screens/          # 各功能页
│       ├── components/       # 可复用组件
│       ├── Theme.kt          # 主题
│       └── AppNavigation.kt  # 导航
├── viewmodel/
│   ├── trusted/              # ApexViewModel, ScanViewModel
│   ├── SettingsViewModel.kt
│   └── UpdateViewModel.kt
├── data/
│   ├── jni/NativeBridge.kt   # JNI 桥接
│   ├── repository/           # Repository
│   ├── updater/              # 软件更新
│   └── ...
├── core/
│   ├── NativeLibraryLoader.kt
│   ├── permission/
│   ├── security/
│   └── notification/
├── domain/                   # 领域模型
├── island/                   # 沙箱
├── game/                     # 游戏模式
├── guard/                    # 实时防护
├── hid/                      # HWID 伪装
└── cure/                     # 修复
```

### 文件命名规范

- **Screen**: `XxxScreen.kt`（如 `DashboardScreen.kt`）
- **ViewModel**: `XxxViewModel.kt`（如 `ApexViewModel.kt`）
- **Repository**: `XxxRepository.kt` 或 `XxxRepositoryImpl.kt`
- **数据类**: `XxxState.kt` 或 `XxxInfo.kt`
- **C++ 检测层**: `layer<N>_<name>.cpp/.h`（如 `layer1_prop.cpp`）
- **C++ 微服务**: `ms<NNN>_<name>/plugin.cpp`

---

## 📝 代码规范

### Kotlin 代码

#### 1. ViewModel 规范

```kotlin
class XxxViewModel(application: Application) : AndroidViewModel(application) {
    // 1. 异常处理器（必须）
    private val exceptionHandler = CoroutineExceptionHandler { _, e ->
        Log.e("XxxViewModel", "Uncaught coroutine exception", e)
    }

    // 2. StateFlow（不可变，对外暴露 StateFlow）
    private val _uiState = MutableStateFlow(XxxUiState())
    val uiState: StateFlow<XxxUiState> = _uiState.asStateFlow()

    // 3. 所有协程必须用 exceptionHandler + Dispatchers.IO
    fun doSomething() {
        viewModelScope.launch(Dispatchers.IO + exceptionHandler) {
            try {
                // ...
            } catch (e: Throwable) {
                Log.e("XxxViewModel", "doSomething failed", e)
            }
        }
    }
}
```

#### 2. Compose UI 规范

```kotlin
@Composable
fun XxxScreen(
    // 1. 回调用高阶函数，可空参数默认 null
    onBack: () -> Unit,
    onNavigateToYyy: (() -> Unit)? = null,
    // 2. ViewModel 通过参数注入，不用 hilt
    viewModel: XxxViewModel = viewModel()
) {
    // 3. 状态收集用 collectAsState
    val uiState by viewModel.uiState.collectAsState()

    // 4. 副作用用 LaunchedEffect，key 要精确
    LaunchedEffect(Unit) {
        // 一次性初始化
    }

    Scaffold { padding ->
        // 5. 主体内容
    }
}
```

#### 3. Native 调用规范

```kotlin
// 所有 native 调用必须通过 NativeLibraryLoader.safeCall
fun detectSomething(): Boolean = NativeLibraryLoader.safeCall(false) {
    detectSomethingNative()
}

// private external 方法声明
private external fun detectSomethingNative(): Boolean
```

### C++ 代码规范

#### 1. 检测层规范

```cpp
// layerX_name.h
#pragma once
namespace apex {
namespace layerX {
    bool detect_something();
    void full_scan(char* report, size_t size);
}
}

// layerX_name.cpp
#include "layerX_name.h"
#include "../common/utils.h"

namespace apex {
namespace layerX {

bool detect_something() {
    // 实现
    return false;
}

void full_scan(char* report, size_t size) {
    snprintf(report, size, "Layer X: %s",
             detect_something() ? "DETECTED" : "CLEAN");
}

} // namespace layerX
} // namespace apex
```

#### 2. JNI 桥接规范

```cpp
// native-lib.cpp
JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectSomethingNative(JNIEnv*, jobject) {
    return apex::layerX::detect_something();
}
```

---

## 🐛 调试技巧

### 1. 日志查看

```bash
# APEX 应用日志
adb logcat -s ApexViewModel:V ScanViewModel:V NativeLoader:V

# Native 日志
adb logcat -s APEX-NATIVE:V

# 更新模块日志
adb logcat -s AppUpdater:V UpdateViewModel:V

# 隐藏模式日志
adb logcat -s HideModeManager:V

# 系统崩溃日志
adb logcat -s AndroidRuntime:E
```

### 2. 崩溃报告

应用内崩溃会自动保存到 `/data/data/com.apex.root/files/crashes/`：

```bash
# 拉取崩溃日志
adb shell run-as com.apex.root ls files/crashes/
adb shell run-as com.apex.root cat files/crashes/crash_<timestamp>.log
```

### 3. Native 调试

```bash
# 启动 native debug（需 Android Studio）
# Run → Debug 'app' → 选择 Native 配置

# 或用 LLDB
adb forward tcp:12345 tcp:12345
lldb-server platform --listen "*:12345"
```

### 4. 性能分析

```bash
# CPU profiler
adb shell dumpsys gfxinfo com.apex.root

# 内存
adb shell dumpsys meminfo com.apex.root

# 启动时间
adb shell am start -W com.apex.root/.ui.MainActivity
```

### 5. 检查原生库

```bash
# 检查 .so 是否在 APK 中
unzip -l app/build/outputs/apk/debug/app-arm64-v8a-debug.apk | grep "\.so"

# 检查 native 库符号
nm -D app/src/main/cpp/build/intermediates/cmake/debug/obj/arm64-v8a/libapex_root.so | grep Java_
```

### 6. 检查 Magisk 模块状态

```bash
adb shell su -c "cat /data/adb/modules/apex-root/module.prop"
adb shell getprop | grep apex
adb shell su -c "cat /data/adb/apex-root/install.log"
adb shell su -c "cat /data/adb/apex-root/log/service.log"
```

---

## 🧪 测试

### 单元测试

```bash
# 运行所有
./gradlew test

# 运行特定类
./gradlew test --tests "com.apex.root.data.DetectionCacheTest"

# 测试报告
open app/build/reports/tests/test/index.html
```

### 测试规范

```kotlin
// src/test/java/.../DetectionCacheTest.kt
class DetectionCacheTest {
    @Test
    fun `put and get string should return same value`() {
        val cache = DetectionCache
        cache.putString("key", "value")
        assertEquals("value", cache.getString("key"))
    }

    @Test
    fun `expired entries should return null`() {
        // 测试 TTL
    }
}
```

### Instrumentation 测试

```bash
# 连接设备后
./gradlew connectedAndroidTest
```

---

## 🔄 贡献流程

### 1. Fork & Clone

```bash
# Fork 仓库到自己的 GitHub 账户
git clone https://github.com/你的用户名/root-check.git
cd root-check
git remote add upstream https://github.com/mengjinghao/root-check.git
```

### 2. 创建分支

```bash
git checkout -b feature/your-feature
# 或
git checkout -b fix/your-bugfix
```

### 3. 开发 & 提交

```bash
# 开发...
git add -A
git commit -m "feat: add xxx feature

详细描述：
- 实现了 X
- 修复了 Y
- 添加了 Z"
```

### 4. 同步上游

```bash
git fetch upstream
git rebase upstream/main
# 解决冲突后
git push origin feature/your-feature
```

### 5. 提交 PR

在 GitHub 上创建 Pull Request 到 `mengjinghao/root-check:main`。

---

## 📋 提交信息规范

使用 Conventional Commits：

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Type

| Type | 说明 |
|------|------|
| `feat` | 新功能 |
| `fix` | Bug 修复 |
| `docs` | 文档更新 |
| `style` | 代码格式（不影响功能） |
| `refactor` | 重构（无新功能、无 bug 修复） |
| `perf` | 性能优化 |
| `test` | 测试相关 |
| `chore` | 构建/工具链变更 |
| `ci` | CI 配置 |
| `revert` | 回滚 |

### 示例

```
feat(updater): add Magisk module download support

- Add fetchLatestModuleZip() to scan GitHub releases for .zip assets
- Add downloadModuleZip() with streaming progress
- Add installModuleZip() via FileProvider + ACTION_VIEW
- New ModuleZipInfo data class

Closes #42
```

```
fix(dashboard): resolve LazyColumn duplicate key crash

GlassLogViewerScreen used time+message.hashCode() as key,
which collided when two logs had the same second and message.
Switched to itemsIndexed with index-based key.
```

---

## 🏗️ 添加新检测层

### 步骤 1：创建 C++ 检测层

```cpp
// app/src/main/cpp/detect/layer17_yourname.h
#pragma once
namespace apex {
namespace layer17 {
    bool detect_your_target();
    void full_scan(char* report, size_t size);
}
}
```

```cpp
// app/src/main/cpp/detect/layer17_yourname.cpp
#include "layer17_yourname.h"
#include "../common/utils.h"

namespace apex {
namespace layer17 {

bool detect_your_target() {
    // 检测逻辑
    return utils::file_exists("/data/adb/your_target");
}

void full_scan(char* report, size_t size) {
    snprintf(report, size, "L17 YourTarget: %s",
             detect_your_target() ? "❌ 异常" : "✅ 正常");
}

} // namespace layer17
} // namespace apex
```

### 步骤 2：添加 JNI 桥接

```cpp
// native-lib.cpp
#include "detect/layer17_yourname.h"

JNIEXPORT jboolean JNICALL
Java_com_apex_root_data_jni_NativeBridge_detectYourTargetNative(JNIEnv*, jobject) {
    return apex::layer17::detect_your_target();
}
```

### 步骤 3：添加 Kotlin 方法

```kotlin
// NativeBridge.kt
fun detectYourTarget(): Boolean = NativeLibraryLoader.safeCall(false) {
    detectYourTargetNative()
}
private external fun detectYourTargetNative(): Boolean
```

### 步骤 4：更新评分算法

```cpp
// native-lib.cpp getRiskScoreNative()
if (detectYourTarget()) score += 8;
```

### 步骤 5：更新扫描结果

```cpp
// native-lib.cpp runQuickScanNative()
bool l17 = detectYourTarget();
result += "L17 YourTarget: " + std::string(l17 ? "❌ 异常" : "✅ 正常") + "\n";
```

### 步骤 6：添加修复建议

```kotlin
// FixRecommendations.kt
"YourTarget" to FixRecommendation(
    id = "your_target",
    titleZh = "YourTarget 检测异常",
    descriptionZh = "...",
    stepsZh = listOf("1. ...", "2. ..."),
    priority = 7
)
```

### 步骤 7：更新 parseScanLayers

```kotlin
// ApexViewModel.kt parseScanLayers()
line.contains("YourTarget") -> layers.add("YourTarget")
```

---

## 📚 相关文档

- [环境搭建](./BUILD_ENV.md)
- [构建指南](./BUILD.md)
- [故障排查](./TROUBLESHOOTING.md)
- [架构设计](./ARCHITECTURE.md)
