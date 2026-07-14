# 故障排查指南

本文档汇总 APEX-Root 开发、构建、运行过程中的常见问题及解决方案。

---

## 📑 目录

- [构建期问题](#构建期问题)
- [安装期问题](#安装期问题)
- [运行期问题（闪退）](#运行期问题闪退)
- [原生库问题](#原生库问题)
- [Root 检测问题](#root-检测问题)
- [隐藏模式问题](#隐藏模式问题)
- [网络与更新问题](#网络与更新问题)
- [性能问题](#性能问题)
- [调试技巧](#调试技巧)

---

## 构建期问题

### Q1: `jlink does not exist`

**完整错误**：
```
Execution failed for task ':app:compileDebugJavaWithJavac'.
> Could not resolve all files for configuration ':app:androidJdkImage'.
   > Failed to transform core-for-system-modules.jar
      > jlink executable /usr/lib/jvm/java-21-openjdk-amd64/bin/jlink does not exist.
```

**原因**：系统安装了 JRE 而非完整 JDK，或 JDK 版本不是 17。

**解决**：

```bash
# 1. 检查当前 JDK
java -version
which jlink  # 应有输出，空则缺少

# 2. 安装 JDK 17（完整版，含 jlink）
# Linux
sudo apt install -y openjdk-17-jdk-headless

# macOS
brew install --cask temurin@17

# 手动下载（无 sudo）
wget https://api.adoptium.net/v3/binary/latest/17/ga/linux/x64/jdk/hotspot/normal/eclipse?project=jdk -O jdk17.tar.gz
mkdir -p $HOME/jdk17
tar xzf jdk17.tar.gz -C $HOME/jdk17 --strip-components=1

# 3. 设置 JAVA_HOME
export JAVA_HOME=$HOME/jdk17  # 或 /usr/lib/jvm/java-17-openjdk-amd64
export PATH=$JAVA_HOME/bin:$PATH

# 4. 验证
jlink --version  # 应输出版本号
```

---

### Q2: `SDK location not found`

**完整错误**：
```
SDK location not found. Define a valid SDK location with an ANDROID_HOME environment variable or by setting the sdk.dir path in your project's local properties file.
```

**解决**：

```bash
# 方法 1：创建 local.properties
cd apex-root
echo "sdk.dir=/home/用户/android-sdk" > local.properties

# 方法 2：设置环境变量
export ANDROID_HOME=/home/用户/android-sdk
export ANDROID_SDK_ROOT=$ANDROID_HOME

# 验证 SDK 已安装
ls $ANDROID_HOME/platforms/  # 应有 android-34
ls $ANDROID_HOME/ndk/        # 应有 28.2.13676358
```

---

### Q3: `NDK not configured` 或 `No version of NDK matched`

**原因**：NDK 未安装或版本不匹配。

**解决**：

```bash
# 1. 检查 build.gradle.kts 要求的版本
grep ndkVersion app/build.gradle.kts
# 输出：ndkVersion = "28.2.13676358"

# 2. 安装对应版本
sdkmanager "ndk;28.2.13676358"

# 3. 验证
ls $ANDROID_HOME/ndk/
# 应输出：28.2.13676358

# 4. 如果版本不同，修改 build.gradle.kts 中的 ndkVersion 匹配已安装版本
```

---

### Q4: CMake 编译错误 `C/C++: error:`

**常见 C++ 编译错误**：

#### 4a: `fatal error: 'jni.h' file not found`

**原因**：NDK include 路径未正确配置。

**解决**：
```bash
# 清理 native 缓存
rm -rf app/.cxx
./gradlew assembleDebug
```

#### 4b: `warning: 'execute' has C-linkage specified, but returns user-defined type`

**说明**：这是警告，不是错误，不影响构建。可忽略。

#### 4c: `error: use of undeclared identifier 'bs_clock_ns'`

**原因**：bare_syscall 模块未正确包含。

**解决**：检查 `CMakeLists.txt` 中的 include 路径：
```bash
grep -A5 "CORE_INCLUDE_DIRS" app/src/main/cpp/CMakeLists.txt
# 应包含 bare_syscall 目录
```

---

### Q5: `LintVitalRelease` 失败

**完整错误**：
```
> Task :app:lintVitalRelease FAILED
Error: Subdirectories are not allowed for domain database [FullBackupContent]
```

**解决**：修复对应的 lint 错误。常见：

```bash
# 查看详细 lint 报告
./gradlew lintVitalRelease
# 报告位于：app/build/reports/lint-results-debug.html

# 修复后重新构建
./gradlew assembleRelease
```

**快速绕过**（不推荐，仅临时）：在 `app/build.gradle.kts` 中：
```kotlin
android {
    lint {
        abortOnError = false
        checkReleaseBuilds = false
    }
}
```

---

### Q6: `OutOfMemoryError` 构建时

**解决**：增加 Gradle 内存

```bash
# 临时
./gradlew assembleDebug -Dorg.gradle.jvmargs="-Xmx4096m"

# 永久（gradle.properties）
echo "org.gradle.jvmargs=-Xmx4096m -XX:MaxMetaspaceSize=512m" >> gradle.properties
```

---

### Q7: Protobuf 生成失败

**错误**：
```
> Task :app:generateDebugProto FAILED
```

**解决**：
```bash
# 清理 protobuf 缓存
rm -rf app/build/generated/source/proto
./gradlew assembleDebug
```

---

### Q8: `Could not resolve all files` 网络问题

**解决**：配置镜像源（国内用户）

编辑 `settings.gradle.kts`：
```kotlin
dependencyResolutionManagement {
    repositories {
        maven { url = uri("https://maven.aliyun.com/repository/public") }
        maven { url = uri("https://maven.aliyun.com/repository/google") }
        google()
        mavenCentral()
    }
}
```

---

## 安装期问题

### Q9: `INSTALL_FAILED_NO_MATCHING_ABIS`

**原因**：APK 架构与设备不匹配（如安装 x86_64 APK 到 ARM 手机）。

**解决**：
```bash
# 查看设备架构
adb shell getprop ro.product.cpu.abi
# 输出如：arm64-v8a

# 安装对应架构的 APK
adb install app-arm64-v8a-debug.apk
```

---

### Q10: `INSTALL_FAILED_UPDATE_INCOMPATIBLE`

**原因**：已安装相同包名但签名不同的版本。

**解决**：
```bash
# 卸载旧版本
adb uninstall com.apex.root

# 重新安装
adb install app-arm64-v8a-debug.apk
```

---

### Q11: `INSTALL_PARSE_FAILED_NO_CERTIFICATES`

**原因**：APK 未签名或签名损坏。

**解决**：
```bash
# 检查 APK 签名
$ANDROID_HOME/build-tools/34.0.0/apksigner verify --verbose app-arm64-v8a-debug.apk

# 如未签名，重新构建
./gradlew assembleDebug
```

---

## 运行期问题（闪退）

### Q12: 启动即闪退

**排查步骤**：

```bash
# 1. 查看 crash 日志
adb logcat -s AndroidRuntime:E | tail -50

# 2. 常见原因
# a) UnsatisfiedLinkError: native 库未加载
# b) IllegalStateException: Compose key 冲突
# c) RuntimeException: SharedPreferences 损坏
```

#### 12a: `UnsatisfiedLinkError: dlopen failed`

**原因**：原生库加载失败。

**解决**：
```bash
# 查看详细 dlopen 错误
adb logcat -s APEX-NATIVE:V NativeLoader:V | tail -20

# 常见原因：
# - 设备非 ARM64（libapex_root.so 仅编译 arm64-v8a/armeabi-v7a/x86_64）
# - APK 安装不完整 → 重新安装
# - Android 版本过低（需 10+ / API 29+）
```

#### 12b: `IllegalArgumentException: Key X was already used`

**原因**：LazyColumn 重复 key。

**解决**：此问题已在 v1.0.3+ 修复。如仍出现，请更新到最新版本。

#### 12c: `IllegalStateException: Sheet was already dismissed`

**原因**：ModalBottomSheet 移除过早。

**解决**：此问题已在 v1.0.3+ 修复。

---

### Q13: 扫描时闪退

**排查**：
```bash
adb logcat -s APEX-NATIVE:V ApexViewModel:V | tail -30
```

**常见原因**：
- 原生库未加载 → 在「软件更新 → 原生引擎诊断」点击「重试加载」
- SELinux 限制 → 检查设备是否 enforcing
- 内存不足 → 关闭其他应用后重试

---

### Q14: 切换隐藏模式闪退

**排查**：
```bash
adb logcat -s APEX-NATIVE:V HideModeManager:V | tail -30
```

**常见原因**：
- 原生库未加载 → 同 Q12a
- 未 root → 检查 `su -c id` 是否返回 uid=0
- SELinux 拒绝 → 检查 sepolicy.rule 是否安装

---

## 原生库问题

### Q15: 原生库加载失败

**症状**：UI 显示「原生检测引擎状态: 加载失败」

**排查**：
```bash
adb logcat -s NativeLoader:E APEX-NATIVE:E | tail -20
```

**常见原因与解决**：

| 原因 | 解决 |
|------|------|
| 设备非 ARM64 | 安装对应架构 APK（arm64-v8a/armeabi-v7a/x86_64） |
| SELinux 临时拒绝 | 在「软件更新 → 原生引擎诊断」点击「重试加载」 |
| APK 损坏 | 卸载重装 |
| Android < 10 | 升级系统或使用支持的设备 |

---

### Q16: `detectSuspiciousProperties` 返回错误

**排查**：
```bash
adb logcat -s APEX-NATIVE:V | grep "L1"
```

**解决**：通常是正常情况（设备确实有 root 痕迹），非 bug。

---

## Root 检测问题

### Q17: 检测不到 Root（但设备已 Root）

**原因**：Magisk DenyList / Shamiko 隐藏了 APEX-Root。

**解决**：
1. 打开 Magisk Manager
2. 设置 → DenyList → 移除 `com.apex.root`
3. 如使用 Shamiko，将其配置为白名单模式
4. 重启 APEX-Root 重新扫描

---

### Q18: 检测到 Root 但设备未 Root（误报）

**可能原因**：
- 自定义 ROM 包含 su 二进制（如 LineageOS 的开发版）
- 旧版 Magisk 残留文件未清理

**排查**：
```bash
adb shell su -c "ls /data/adb/"
# 查看是否有 magisk / ksu / ap 目录
```

---

### Q19: 侧信道检测时延异常

**原因**：设备负载高导致 syscall 时延波动。

**解决**：关闭后台应用后重试，或切换到「快速扫描」模式。

---

## 隐藏模式问题

### Q20: Hide 模式无效（其他应用仍能检测到 Root）

**排查**：
```bash
# 检查隐藏模式状态
adb shell getprop apex.fw.mode
adb shell getprop apex.fw.ns
adb shell getprop apex.hide_daemon.state
```

**解决**：
1. 确认已安装 Magisk 模块（`/data/adb/modules/apex-root/`）
2. 重启设备
3. 在 APEX-Root 中切换到 Hide 模式
4. 检查目标应用是否在白名单中

---

### Q21: Game 模式导致应用崩溃

**原因**：Game 模式隐藏过于激进，部分应用依赖被隐藏的服务。

**解决**：
1. 切换回 Hide 或 Detection 模式
2. 在「设置 → 隐藏策略」中调整为 TARGETED（仅针对特定应用）
3. 将崩溃应用加入白名单

---

### Q22: 切换模式后无法恢复

**解决**：
```bash
# 强制停止隐藏
adb shell su -c "setprop apex.fw.mode detection"
adb shell su -c "killall apex_hide_daemon 2>/dev/null"

# 重启 APEX-Root
adb shell am force-stop com.apex.root
```

---

## 网络与更新问题

### Q23: 检查更新失败

**错误信息**：「无法连接到 GitHub（DNS 解析失败）」

**解决**：
1. 检查网络连接
2. 如在国内，使用 VPN 或配置代理
3. 在「设置 → 更新 → 更新代理」中配置代理
4. 手动访问 https://github.com/mengjinghao/root-check/releases

---

### Q24: 下载 APK 失败

**排查**：
```bash
adb logcat -s AppUpdater:V | tail -20
```

**常见原因**：
- 非 Wi-Fi 网络（设置了仅 Wi-Fi 下载）
- GitHub 限流
- 存储空间不足

**解决**：
- 关闭「仅 Wi-Fi 下载」设置
- 清理缓存
- 手动在浏览器下载

---

### Q25: 模块查询返回「未找到模块 zip」

**原因**：GitHub Release 中没有文件名包含 `module`/`magisk` 的 .zip 附件。

**解决**：
1. 本地构建模块：`./scripts/build_module.sh --version 1.0.4`
2. 上传到 GitHub Release
3. 在 APEX-Root 中重新查询

---

## 性能问题

### Q26: 扫描很慢（>30 秒）

**原因**：深度扫描包含 16 层 + 侧信道时延测量，正常需要 10-30 秒。

**优化**：
1. 使用「快速扫描」（<500ms）
2. 关闭侧信道检测（设置 → 检测级别 → 快速）
3. 在「设置 → 性能」中调整 `scanConcurrency`

---

### Q27: 应用卡顿/ANR

**排查**：
```bash
adb logcat -s ActivityManager:W | grep "ANR"
```

**常见原因**：
- 主线程磁盘 IO（已在 v1.0.3+ 修复）
- 原生库加载阻塞主线程（已在 v1.0.3+ 修复）
- 内存不足

**解决**：
1. 更新到最新版本
2. 清理应用缓存
3. 重启设备

---

### Q28: 内存占用过高

**排查**：
```bash
adb shell dumpsys meminfo com.apex.root | head -20
```

**解决**：
1. 退出后重新进入（清理 UI state）
2. 在「设置 → 性能」中启用「低内存模式」
3. 减少日志保留数量

---

## 调试技巧

### 抓取完整日志

```bash
# 保存到文件
adb logcat > apex_log.txt

# 过滤 APEX 相关
adb logcat -s APEX-NATIVE:V ApexViewModel:V NativeLoader:V AppUpdater:V

# 查看 crash 报告
adb logcat -s AndroidRuntime:E
```

### 查看应用崩溃文件

```bash
# 应用内崩溃日志
adb shell run-as com.apex.root ls files/crashes/
adb shell run-as com.apex.root cat files/crashes/crash_*.log
```

### 检查原生库是否加载

```bash
# 进入应用后
adb shell ps | grep apex
adb shell cat /proc/$(adb shell pidof com.apex.root)/maps | grep apex_root
# 应输出 libapex_root.so 的内存映射
```

### 检查 Root 状态

```bash
# 检查 su 可用
adb shell su -c id
# 应输出：uid=0(root)...

# 检查 Magisk
adb shell su -c "magisk -v"
adb shell su -c "ls /data/adb/modules/"

# 检查 SELinux
adb shell getenforce
# Enforcing / Permissive
```

### 检查隐藏模式状态

```bash
adb shell getprop | grep apex
# 输出：
# [apex.fw.mode]: [ebpf]
# [apex.fw.ns]: [ok]
# [apex.module.state]: [installed]
# [apex.module.version]: [v1.0.4]
```

### 性能分析

```bash
# CPU profiler
adb shell dumpsys gfxinfo com.apex.root

# 内存 profiler
adb shell dumpsys meminfo com.apex.root

# 启动时间
adb shell am start -W com.apex.root/.ui.MainActivity
```

---

## 📚 相关文档

- [环境搭建](./BUILD_ENV.md)
- [构建指南](./BUILD.md)
- [架构设计](./ARCHITECTURE.md)
- [开发指南](./DEVELOPMENT.md)

如本文档未覆盖您的问题，请提交 [GitHub Issue](https://github.com/mengjinghao/root-check/issues) 并附上：
1. 设备型号 + Android 版本
2. APEX-Root 版本（关于页可见）
3. `adb logcat` 完整日志
4. 复现步骤
