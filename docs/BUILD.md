# 构建指南

本文档详细介绍 APEX-Root 的构建方法，包括 debug/release 构建、APK 签名、ABI 分包、模块打包等。

**前置条件**：请先完成 [BUILD_ENV.md](./BUILD_ENV.md) 中的环境搭建。

---

## 📋 构建产物概览

| 产物 | 用途 | 路径 |
|------|------|------|
| Debug APK | 开发调试 | `app/build/outputs/apk/debug/app-<abi>-debug.apk` |
| Release APK | 发布分发 | `app/build/outputs/apk/release/app-<abi>-release.apk` |
| Magisk 模块 ZIP | 开机自动隐藏 | `build/output/apex-root-module-vX.Y.Z.zip` |
| Daemon 模块 ZIP | 独立守护进程 | `build/output/apex-hide-daemon-module-vX.Y.Z.zip` |

---

## 🚀 快速开始（5 分钟构建）

### 1. Debug 构建（最快）

```bash
cd root-check/apex-root
./gradlew assembleDebug
```

构建完成后，APK 位于：

```
app/build/outputs/apk/debug/
├── app-arm64-v8a-debug.apk      # ARM 64位（推荐手机）
├── app-armeabi-v7a-debug.apk    # ARM 32位（旧手机）
├── app-x86_64-debug.apk         # x86 64位（模拟器）
└── output-metadata.json
```

### 2. 安装到设备

```bash
# 连接设备（开启 USB 调试）
adb devices

# 安装对应架构的 APK（推荐 arm64-v8a）
adb install app/build/outputs/apk/debug/app-arm64-v8a-debug.apk
```

---

## 🔧 详细构建配置

### ABI 分包说明

项目使用 `splits.abi` 生成按架构分包的独立 APK：

```kotlin
// app/build.gradle.kts
splits {
    abi {
        isEnable = true
        reset()
        include("arm64-v8a", "armeabi-v7a", "x86_64")
        isUniversalApk = false  // 不生成通用 APK
    }
}
```

**如何选择正确的 APK**：

| 设备类型 | 架构 | 对应 APK |
|----------|------|----------|
| 现代手机（2020+） | arm64-v8a | `app-arm64-v8a-*.apk` |
| 旧款手机（2018 前） | armeabi-v7a | `app-armeabi-v7a-*.apk` |
| Android 模拟器 | x86_64 | `app-x86_64-*.apk` |

**查看设备架构**：

```bash
adb shell getprop ro.product.cpu.abi
# 输出如：arm64-v8a
```

### Debug 构建详解

```bash
# 完整 debug 构建（含 native 编译）
./gradlew assembleDebug

# 仅编译 Kotlin（跳过 native，用于快速语法检查）
./gradlew :app:compileDebugKotlin

# 仅编译 C++（调试 native 代码时）
./gradlew :app:buildCMakeDebug

# 清理后构建
./gradlew clean assembleDebug
```

**Debug 构建特点**：
- 使用默认 debug 签名（自动生成）
- 不启用 ProGuard/R8 混淆
- native 库带调试符号
- APK 体积较大（~55 MB）

### Release 构建详解

#### 步骤 1：生成签名 keystore（仅首次）

```bash
keytool -genkey -v \
    -keystore apex-release.jks \
    -storepass meng411722 \
    -keypass meng411722 \
    -alias root \
    -keyalg RSA -keysize 2048 \
    -validity 10000 \
    -dname "CN=APEX Root, OU=Dev, O=APEX, L=CN, ST=CN, C=CN"
```

⚠️ **安全提示**：
- `apex-release.jks` 必须保密，切勿提交到 git（已在 .gitignore）
- 密码 `meng411722` 是项目默认值，生产环境应修改
- `validity 10000` = 约 27 年有效期

#### 步骤 2：执行 release 构建

```bash
./gradlew assembleRelease
```

构建完成后，APK 位于：

```
app/build/outputs/apk/release/
├── app-arm64-v8a-release.apk      # ~4.5 MB（已混淆+压缩）
├── app-armeabi-v7a-release.apk    # ~4.2 MB
└── app-x86_64-release.apk         # ~4.5 MB
```

**Release 构建特点**：
- 启用 ProGuard/R8 混淆（`isMinifyEnabled = true`）
- 启用资源压缩（`shrinkResources`）
- native 库 strip 调试符号
- APK 体积大幅缩小（debug 55MB → release 4.5MB）

#### 步骤 3：验证签名

```bash
# 验证 APK 签名
$ANDROID_HOME/build-tools/34.0.0/apksigner verify --verbose app-arm64-v8a-release.apk
# 应输出：Verifies

# 查看签名信息
keytool -printcert -jarfile app-arm64-v8a-release.apk
```

---

## 📦 构建变体

### 仅构建特定 ABI

```bash
# 仅 arm64-v8a（加快构建速度）
./gradlew assembleDebug -Pandroid.injected.build.abi=arm64-v8a

# 但 splits.abi 仍会生成 3 个 APK，只是其他 ABI 跳过编译
```

### 构建特定变体

```bash
# 列出所有变体
./gradlew tasks --group="build"

# 常用变体
./gradlew assembleArm64-v8aDebug      # 仅 arm64 debug
./gradlew assembleArm64-v8aRelease    # 仅 arm64 release
./gradlew assembleX86_64Debug         # 仅 x86_64 debug（模拟器）
```

---

## 🧩 构建 Magisk 模块

### 使用 Linux/Mac 脚本（推荐）

```bash
cd root-check/apex-root

# 构建主模块（apex-root）
./scripts/build_module.sh

# 构建守护进程模块（apex-hide-daemon）
./scripts/build_module.sh --daemon

# 构建所有模块
./scripts/build_module.sh --all

# 指定版本号
./scripts/build_module.sh --version 1.0.4
```

输出位于 `build/output/`：

```
build/output/
├── apex-root-module-v1.0.4.zip
└── apex-hide-daemon-module-v1.0.4.zip
```

### 使用 Windows PowerShell

```powershell
cd apex-root
.\scripts\build_module.ps1
```

### 模块 ZIP 结构

```
apex-root-module-v1.0.4.zip
├── module.prop           # 模块元信息
├── customize.sh          # 安装脚本
├── service.sh            # 开机服务
├── post-fs-data.sh       # 早期挂载
├── uninstall.sh          # 卸载脚本
├── update.json           # 更新源信息
├── sepolicy.rule         # SELinux 策略
└── bpf/
    └── apex_firewall.bpf.o  # eBPF 程序（如已编译）
```

---

## 🔄 增量构建加速

### 1. 启用 Gradle 守护进程

```bash
# gradle.properties 中应有
org.gradle.daemon=true
org.gradle.parallel=true
org.gradle.caching=true
org.gradle.configuration-cache=true
```

### 2. 使用 ccache 加速 C++ 编译

```bash
# 安装
sudo apt install -y ccache  # Linux
brew install ccache         # macOS

# 配置 ccache
ccache --set-config=cache_dir=$HOME/.ccache
ccache --set-config=max_size=5G

# 在 app/build.gradle.kts 中添加
externalNativeBuild {
    cmake {
        arguments += "-DCMAKE_C_COMPILER_LAUNCHER=ccache"
        arguments += "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
    }
}
```

### 3. 构建缓存

```bash
# 首次构建（慢）
./gradlew assembleDebug  # 约 5-10 分钟

# 后续增量构建（快）
./gradlew assembleDebug  # 约 30-60 秒（仅改动部分）
```

---

## 📊 构建性能参考

| 构建类型 | 首次构建 | 增量构建 | 产物大小 |
|----------|----------|----------|----------|
| Debug (全 ABI) | 8-12 分钟 | 1-2 分钟 | 3 × 55 MB |
| Debug (仅 arm64) | 5-8 分钟 | 30-60 秒 | 1 × 55 MB |
| Release (全 ABI) | 12-18 分钟 | 3-5 分钟 | 3 × 4.5 MB |
| Release (仅 arm64) | 8-12 分钟 | 2-3 分钟 | 1 × 4.5 MB |
| 仅 Kotlin 编译 | 2-3 分钟 | 20-30 秒 | 无 APK |
| Magisk 模块 | 5 秒 | 5 秒 | 1 × 50 KB |

*测试环境：Linux x86_64, 16GB RAM, SSD, 8 核 CPU*

---

## 🏷️ 版本号管理

### 修改版本号

编辑 `app/build.gradle.kts`：

```kotlin
defaultConfig {
    versionCode = 104      // 整数版本号（每次发布递增）
    versionName = "1.0.4"  // 显示版本号
}
```

### 版本号规则

- `versionCode`：整数，每次发布必须递增（用于 Google Play 判断升级）
- `versionName`：字符串，用户可见（如 `1.0.4`、`1.1.0-beta`）

### 发布新版本流程

```bash
# 1. 更新版本号
sed -i 's/versionCode = 104/versionCode = 105/' app/build.gradle.kts
sed -i 's/versionName = "1.0.4"/versionName = "1.0.5"/' app/build.gradle.kts

# 2. 构建 release APK
./gradlew assembleRelease

# 3. 构建模块
./scripts/build_module.sh --version 1.0.5

# 4. 提交
git add -A
git commit -m "release: v1.0.5"
git tag v1.0.5
git push origin main --tags

# 5. 创建 GitHub Release（使用 gh CLI）
gh release create v1.0.5 \
    app/build/outputs/apk/release/app-arm64-v8a-release.apk \
    app/build/outputs/apk/release/app-armeabi-v7a-release.apk \
    app/build/outputs/apk/release/app-x86_64-release.apk \
    build/output/apex-root-module-v1.0.5.zip \
    --title "APEX-Root v1.0.5" \
    --notes "Release notes here"
```

---

## 🧪 测试构建

### 单元测试

```bash
# 运行所有单元测试
./gradlew test

# 运行特定测试类
./gradlew test --tests "com.apex.root.data.DetectionCacheTest"

# 生成测试报告
./gradlew test
open app/build/reports/tests/test/index.html  # macOS
xdg-open app/build/reports/tests/test/index.html  # Linux
```

### Instrumentation 测试（需连接设备）

```bash
# 连接设备
adb devices

# 运行 instrumentation 测试
./gradlew connectedAndroidTest
```

---

## 🚨 常见构建问题

### 问题 1：`LintVitalRelease` 失败

**原因**：lint 检查发现严重问题。

**解决**：
```bash
# 查看详细错误
./gradlew lintVitalRelease

# 修复后重新构建
./gradlew assembleRelease
```

### 问题 2：`Unable to strip libraries`

**警告**：native 库无法 strip（不影响功能，仅 APK 偏大）。

**解决**：可忽略，或手动 strip：
```bash
$ANDROID_HOME/ndk/28.2.13676358/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip \
    --strip-unneeded app/src/main/cpp/lib*.so
```

### 问题 3：`OutOfMemoryError: Java heap space`

**解决**：增加 Gradle 内存
```bash
./gradlew assembleDebug -Dorg.gradle.jvmargs="-Xmx4096m"
```

### 问题 4：CMake 编译失败

**解决**：
```bash
# 清理 native 构建缓存
rm -rf app/.cxx
rm -rf app/build/intermediates/cmake

# 重新构建
./gradlew assembleDebug
```

### 问题 5：`UnsatisfiedLinkError` 运行时

**原因**：APK 中缺少对应架构的 .so 文件。

**排查**：
```bash
# 检查 APK 中的 .so 文件
unzip -l app-arm64-v8a-debug.apk | grep "\.so"
# 应输出 lib/arm64-v8a/libapex_root.so
```

---

## 📚 相关文档

- [环境搭建](./BUILD_ENV.md)
- [故障排查](./TROUBLESHOOTING.md)
- [架构设计](./ARCHITECTURE.md)
- [开发指南](./DEVELOPMENT.md)
