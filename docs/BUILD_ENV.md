# 构建环境搭建指南

本文档详细介绍从零搭建 APEX-Root 编译环境的方法，覆盖 Windows / macOS / Linux 三大平台。

---

## 📋 环境要求总览

| 组件 | 要求版本 | 用途 |
|------|----------|------|
| **JDK** | 17 (Temurin/OpenJDK) | Gradle 8.2 + Kotlin 1.9 编译 |
| **Android SDK** | Platform 34 + Build Tools 34.0.0 | Android 编译 |
| **Android NDK** | 28.2.13676358 | C++ 原生代码编译 |
| **CMake** | 3.22.1 (Android SDK 内置) | C++ 构建系统 |
| **Gradle** | 8.2 (项目自带 wrapper) | 构建工具 |
| **Kotlin** | 1.9.20 (Gradle 插件管理) | 主语言 |
| **Protobuf** | 3.25.1 (Gradle 插件管理) | IPC 序列化 |
| **Git** | 2.20+ | 代码版本控制 |

**磁盘空间要求**：约 8 GB（SDK 4GB + NDK 3GB + Gradle 缓存 1GB）

**内存要求**：建议 8 GB+（Gradle + Kotlin 编译 + CMake 并发）

---

## 🖥️ 平台选择建议

| 平台 | 推荐度 | 说明 |
|------|--------|------|
| **Linux (Ubuntu 22.04+)** | ⭐⭐⭐⭐⭐ | 原生支持，编译速度最快，CI 友好 |
| **macOS (Intel/Apple Silicon)** | ⭐⭐⭐⭐ | 原生支持，Apple Silicon 需注意 NDK 路径 |
| **Windows 10/11 + WSL2** | ⭐⭐⭐ | 推荐 WSL2 Ubuntu，原生 cmd 需额外配置 |
| **Windows 10/11 原生** | ⭐⭐ | 可用但路径问题多，不推荐 |

---

## 📦 各平台详细安装步骤

### 方式一：Linux (Ubuntu/Debian) — 推荐

#### 1. 安装 JDK 17

```bash
# 方式 A：apt 安装（需要 sudo）
sudo apt update
sudo apt install -y openjdk-17-jdk-headless

# 验证
java -version
# 应输出：openjdk version "17.x.x"
javac -version
jlink --version  # 重要：jlink 必须存在，Gradle 需要它
```

```bash
# 方式 B：手动下载 Temurin（无 sudo 权限时）
cd /tmp
wget https://api.adoptium.net/v3/binary/latest/17/ga/linux/x64/jdk/hotspot/normal/eclipse?project=jdk -O jdk17.tar.gz
mkdir -p $HOME/jdk17
tar xzf jdk17.tar.gz -C $HOME/jdk17 --strip-components=1

# 配置环境变量（写入 ~/.bashrc 或 ~/.zshrc）
echo 'export JAVA_HOME=$HOME/jdk17' >> ~/.bashrc
echo 'export PATH=$JAVA_HOME/bin:$PATH' >> ~/.bashrc
source ~/.bashrc

# 验证
java -version
jlink --version
```

#### 2. 安装 Android SDK

```bash
# 创建 SDK 目录
mkdir -p $HOME/android-sdk

# 下载 commandline tools
cd /tmp
wget https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip -O cmdtools.zip
unzip cmdtools.zip -d $HOME/android-sdk/

# 重组目录结构（sdkmanager 要求 cmdline-tools/latest/）
cd $HOME/android-sdk
mkdir -p cmdline-tools/latest
mv cmdline-tools/bin cmdline-tools/lib cmdline-tools/NOTICE.txt cmdline-tools/source.properties cmdline-tools/latest/

# 配置环境变量
echo 'export ANDROID_HOME=$HOME/android-sdk' >> ~/.bashrc
echo 'export ANDROID_SDK_ROOT=$HOME/android-sdk' >> ~/.bashrc
echo 'export PATH=$PATH:$ANDROID_HOME/cmdline-tools/latest/bin:$ANDROID_HOME/platform-tools' >> ~/.bashrc
source ~/.bashrc

# 接受所有许可
yes | sdkmanager --licenses

# 安装必要组件
sdkmanager "platform-tools"
sdkmanager "platforms;android-34"
sdkmanager "build-tools;34.0.0"
sdkmanager "cmake;3.22.1"
```

#### 3. 安装 NDK 28.2

```bash
# 通过 sdkmanager 安装（推荐）
sdkmanager "ndk;28.2.13676358"

# 验证
ls $ANDROID_HOME/ndk/
# 应输出：28.2.13676358
```

#### 4. 安装其他工具

```bash
# 构建工具
sudo apt install -y build-essential zip unzip git

# 可选：ccache 加速 C++ 重复编译
sudo apt install -y ccache
```

---

### 方式二：macOS

#### 1. 安装 Homebrew（如未安装）

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

#### 2. 安装 JDK 17

```bash
brew install --cask temurin@17

# 验证
/usr/libexec/java_home -V
# 应列出 Temurin 17

# 配置 JAVA_HOME（写入 ~/.zshrc 或 ~/.bash_profile）
echo 'export JAVA_HOME=$(/usr/libexec/java_home -v 17)' >> ~/.zshrc
source ~/.zshrc
```

#### 3. 安装 Android SDK + NDK

```bash
# 方式 A：安装 Android Studio（推荐新手）
brew install --cask android-studio
# 打开 Android Studio → SDK Manager → 安装：
#   - Android SDK Platform 34
#   - NDK 28.2.13676358
#   - CMake 3.22.1
#   - Android SDK Build-Tools 34.0.0

# 方式 B：commandline tools（无 GUI）
brew install --cask android-commandlinetools
yes | sdkmanager --licenses
sdkmanager "platform-tools" "platforms;android-34" "build-tools;34.0.0" "cmake;3.22.1" "ndk;28.2.13676358"
```

#### 4. 配置环境变量

```bash
# Apple Silicon (M1/M2/M3) 路径
echo 'export ANDROID_HOME=$HOME/Library/Android/sdk' >> ~/.zshrc

# Intel Mac 路径相同
echo 'export ANDROID_SDK_ROOT=$ANDROID_HOME' >> ~/.zshrc
echo 'export PATH=$PATH:$ANDROID_HOME/cmdline-tools/latest/bin:$ANDROID_HOME/platform-tools' >> ~/.zshrc
source ~/.zshrc
```

---

### 方式三：Windows + WSL2（推荐 Windows 用户）

#### 1. 启用 WSL2

```powershell
# 以管理员身份运行 PowerShell
wsl --install -d Ubuntu-22.04
wsl --set-default-version 2
```

#### 2. 进入 WSL2 Ubuntu，按「方式一：Linux」步骤安装

```bash
# 在 WSL2 Ubuntu 中
sudo apt update
sudo apt install -y openjdk-17-jdk-headless build-essential zip unzip git
# ... 后续步骤同 Linux
```

#### 3. 访问 Windows 文件系统（可选）

```bash
# WSL2 中访问 Windows 盘
cd /mnt/c/Users/你的用户名/Projects/root-check
```

---

### 方式四：Windows 原生（不推荐）

如必须使用 Windows 原生 cmd/PowerShell：

#### 1. 安装 JDK 17

下载 Temurin 17：https://adoptium.net/temurin/releases/?version=17

```powershell
# 设置环境变量（PowerShell 管理员）
[Environment]::SetEnvironmentVariable("JAVA_HOME", "C:\Program Files\Eclipse Adoptium\jdk-17.x.x-hotspot", "Machine")
[Environment]::SetEnvironmentVariable("Path", $env:Path + ";%JAVA_HOME%\bin", "Machine")
```

#### 2. 安装 Android Studio

下载：https://developer.android.com/studio

通过 SDK Manager 安装：
- Android SDK Platform 34
- NDK 28.2.13676358
- CMake 3.22.1
- Build Tools 34.0.0

#### 3. 配置环境变量

```powershell
[Environment]::SetEnvironmentVariable("ANDROID_HOME", "$env:LOCALAPPDATA\Android\Sdk", "User")
[Environment]::SetEnvironmentVariable("Path", $env:Path + ";%ANDROID_HOME%\platform-tools", "User")
```

⚠️ **Windows 原生已知问题**：
- CMake 路径中的空格可能导致构建失败
- 长路径限制（>260 字符）可能影响 Gradle 缓存
- shell 脚本（`.sh`）无法直接运行，需 Git Bash

---

## 🔧 环境验证

### 一键验证脚本

将以下脚本保存为 `verify_env.sh` 并运行：

```bash
#!/bin/bash
echo "=== APEX-Root 环境验证 ==="

# 1. JDK
echo -n "[1/6] JDK 17: "
if command -v java &>/dev/null; then
    JAVA_VER=$(java -version 2>&1 | head -1 | cut -d'"' -f2 | cut -d'.' -f1)
    if [ "$JAVA_VER" = "17" ]; then
        echo "✓ $(java -version 2>&1 | head -1)"
    else
        echo "✗ 需要 JDK 17，当前为 $JAVA_VER"
    fi
else
    echo "✗ 未找到 java"
fi

# 2. jlink
echo -n "[2/6] jlink: "
if command -v jlink &>/dev/null; then
    echo "✓ $(jlink --version)"
else
    echo "✗ 未找到 jlink（需安装完整 JDK，而非 JRE）"
fi

# 3. Android SDK
echo -n "[3/6] Android SDK: "
if [ -n "$ANDROID_HOME" ] && [ -d "$ANDROID_HOME" ]; then
    echo "✓ $ANDROID_HOME"
else
    echo "✗ ANDROID_HOME 未设置或目录不存在"
fi

# 4. Android Platform 34
echo -n "[4/6] platforms;android-34: "
if [ -d "$ANDROID_HOME/platforms/android-34" ]; then
    echo "✓"
else
    echo "✗ 未安装（运行：sdkmanager \"platforms;android-34\"）"
fi

# 5. NDK
echo -n "[5/6] NDK 28.2: "
if [ -d "$ANDROID_HOME/ndk/28.2.13676358" ]; then
    echo "✓"
else
    echo "✗ 未安装（运行：sdkmanager \"ndk;28.2.13676358\"）"
fi

# 6. CMake
echo -n "[6/6] CMake 3.22.1: "
if [ -d "$ANDROID_HOME/cmake/3.22.1" ]; then
    echo "✓"
else
    echo "✗ 未安装（运行：sdkmanager \"cmake;3.22.1\"）"
fi

echo ""
echo "=== 验证完成 ==="
```

运行：

```bash
chmod +x verify_env.sh
./verify_env.sh
```

所有项显示 ✓ 即环境就绪。

---

## 🏗️ 配置项目

### 1. 克隆仓库

```bash
git clone https://github.com/mengjinghao/root-check.git
cd root-check/apex-root
```

### 2. 创建 local.properties

```bash
# Linux/macOS
echo "sdk.dir=$HOME/android-sdk" > local.properties

# Windows WSL2
echo "sdk.dir=$HOME/android-sdk" > local.properties

# Windows 原生（路径用双反斜杠或正斜杠）
echo "sdk.dir=C:\\Users\\你的用户名\\AppData\\Local\\Android\\Sdk" > local.properties
```

⚠️ **切勿提交 local.properties 到 git**（已在 .gitignore 中）。

### 3. 验证 Gradle 可运行

```bash
chmod +x gradlew
./gradlew --version
```

应输出类似：

```
Gradle 8.2
Kotlin:       1.8.20
Groovy:       3.0.17
JVM:          17.0.x
```

---

## 🚀 常见环境问题排查

### 问题 1：`jlink does not exist`

**原因**：安装了 JRE 而非完整 JDK。

**解决**：
```bash
# 检查
which java
ls /usr/lib/jvm/

# 如果只有 java-21-openjdk-amd64（JRE），安装完整 JDK
sudo apt install -y openjdk-17-jdk-headless  # Linux
# 或手动下载 Temurin 17
```

### 问题 2：`SDK location not found`

**原因**：未创建 `local.properties` 或路径错误。

**解决**：
```bash
cd apex-root
echo "sdk.dir=/绝对路径/到/android-sdk" > local.properties
cat local.properties  # 验证内容
```

### 问题 3：`NDK not configured`

**原因**：NDK 未安装或版本不匹配。

**解决**：
```bash
# 检查 build.gradle.kts 中的 ndkVersion
grep ndkVersion app/build.gradle.kts
# 输出：ndkVersion = "28.2.13676358"

# 安装对应版本
sdkmanager "ndk;28.2.13676358"

# 验证
ls $ANDROID_HOME/ndk/
```

### 问题 4：`CMake not found`

**原因**：CMake 3.22.1 未安装。

**解决**：
```bash
sdkmanager "cmake;3.22.1"
ls $ANDROID_HOME/cmake/
```

### 问题 5：`OutOfMemoryError` 编译时

**原因**：Gradle/JVM 内存不足。

**解决**：编辑 `gradle.properties`：
```properties
org.gradle.jvmargs=-Xmx4096m -XX:MaxMetaspaceSize=512m
org.gradle.parallel=true
org.gradle.caching=true
```

### 问题 6：`Could not resolve all files` 网络问题

**原因**：Gradle 仓库下载失败（网络问题）。

**解决**：使用国内镜像，在 `settings.gradle.kts` 中添加：
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

### 问题 7：Protobuf 插件下载失败

**原因**：`com.google.protobuf:protoc:3.25.1` 下载慢。

**解决**：同问题 6，配置镜像。或手动下载后放入 `~/.gradle/caches/modules-2/files-2.1/`。

---

## 📚 参考资源

- [Android Studio 官方下载](https://developer.android.com/studio)
- [Adoptium JDK 下载](https://adoptium.net/)
- [Android NDK 官方文档](https://developer.android.com/ndk/guides)
- [Gradle 8.2 文档](https://docs.gradle.org/8.2/userguide/userguide.html)
- [Kotlin 1.9 文档](https://kotlinlang.org/docs/home.html)

---

## ✅ 环境检查清单

开始构建前，请确认以下各项全部就绪：

- [ ] JDK 17 已安装，`java -version` 输出 17.x
- [ ] `jlink` 命令可用
- [ ] `ANDROID_HOME` 环境变量已设置
- [ ] `platforms;android-34` 已安装
- [ ] `build-tools;34.0.0` 已安装
- [ ] `ndk;28.2.13676358` 已安装
- [ ] `cmake;3.22.1` 已安装
- [ ] `local.properties` 已创建并指向正确的 SDK 路径
- [ ] `./gradlew --version` 可正常运行
- [ ] 磁盘空间 ≥ 8 GB
- [ ] 内存 ≥ 8 GB（推荐 16 GB）

全部 ✓ 后，请继续阅读 [BUILD.md](./BUILD.md) 进行构建。
