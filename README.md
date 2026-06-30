<div align="center">

```
 █████╗ ███████╗ ██████╗ ███████╗
██╔══██╗██╔════╝██╔═══██╗╚══███╔╝
███████║███████╗██║   ██║  ███╔╝
██╔══██║╚════██║██║   ██║ ███╔╝
██║  ██║███████║╚██████╔╝███████╗
╚═╝  ╚═╝╚══════╝ ╚═════╝ ╚══════╝
```

# APEX-Root · 环境检测

### Android 设备完整性评估系统

[![Android](https://img.shields.io/badge/Android-10%2B-3DDC84?logo=android&logoColor=white)]()
[![API](https://img.shields.io/badge/API-29%2B-critical)]()
[![Arch](https://img.shields.io/badge/Arch-ARM64-blueviolet?logo=arm&logoColor=white)]()
[![Kotlin](https://img.shields.io/badge/Kotlin-1.9-7F52FF?logo=kotlin&logoColor=white)]()
[![Compose](https://img.shields.io/badge/Compose-Material3-7F52FF?logo=jetpackcompose&logoColor=white)]()
[![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)]()
[![NDK](https://img.shields.io/badge/NDK-28.2-FF6F00?logo=androidndk&logoColor=white)]()
[![License](https://img.shields.io/badge/License-Proprietary-red)]()
[![Version](https://img.shields.io/badge/Version-3.1.0-00D4AA)]()

<p>
  <strong>🔍 16 层深度检测 · 🛡️ 3 模式隐藏系统 · 🧬 后量子签名 · ⚡ 微服务架构</strong>
</p>

<p>
  <strong>16-Layer Detection · 3-Mode Hiding · Post-Quantum Signing · Microservice Architecture</strong>
</p>

---

**🇨🇳 [中文](#中文文档) · 🇬🇧 [English](#english-document)**

---

</div>

## 中文文档

### 📖 项目简介

**APEX-Root** 是一款专为 Android 设备完整性评估打造的专业级安全分析工具。它采用 **16 层深度检测架构**，覆盖从系统属性到固件完整性的全维度安全审计，配合 **3 模式隐藏系统**（检测/隐藏/游戏）实现透明化 root 管理。

本项目的核心理念是：**在感知安全与地面实况之间架起桥梁**——为安全研究者、开发者与企业 IT 管理员提供一把分毫不差的诊断仪器。

---

### ✨ 核心特性

#### 🔍 16 层检测架构

| 层级 | 名称 | 检测内容 | 技术实现 |
|------|------|----------|----------|
| **L1** | 系统属性 | `ro.debuggable` / `ro.secure` / `ro.build.tags` | 裸读 `/dev/__properties__` |
| **L2** | ART 注入 | Frida / Xposed / LSPosed / SandHook / Pine | `/proc/self/maps` 扫描 |
| **L3** | 内存特征 | Magisk / Zygisk / Shamiko / Riru 匿名可执行映射 | RWX 页面检测 + 签名匹配 |
| **L4** | 挂载检查 | overlayfs / bind-mount / 命名空间隔离 | `/proc/self/mountinfo` 解析 |
| **L5** | 侧信道 | syscall 时延分析 + 结果一致性检测 | ARM64 裸 `svc` 指令计时 |
| **L6** | Root 守护 | magiskd / ksud / apd / sukid / kitana 进程扫描 | `/proc/*/cmdline` 枚举 |
| **L7** | 启动链 | Bootloader 锁 / AVB / dm-verity / vbmeta | `/proc/cmdline` + sysfs |
| **L8** | Magisk | 主流 + Delta/Kitsune/Kitana fork + DenyList | 守护进程 + 文件路径 |
| **L9** | KernelSU | KSU / SukiSU / KSU-NEXT + Manager APP | 主目录 + 包名检测 |
| **L10** | APatch | APD / KPM 用户态模块 + Manager APP | `/data/adb/ap/` 路径 |
| **L11** | Hook 框架 | Xposed / LSPosed / Frida / Substrate / Epic | 内存映射 + 二进制路径 |
| **L12** | 自定义 ROM | 50+ ROM 标识 (LineageOS / ArrowOS / Rising 等) | build.prop 多分区扫描 |
| **L13** | 固件完整性 | TEE / Modem / Recovery / Vendor 分区 | sysfs + mount 属性 |
| **L14** | 虚拟框架 | VirtualXposed / 太极 / 双开分身 / Island | 包名 + 进程扫描 |
| **L15** | 危险应用 | GameGuardian / CheatEngine / Lucky Patcher | `/data/data` 路径检测 |
| **L16** | Magisk 扩展 | DenyList / ZygiskNext / ReZygisk / LSPosed / Riru | 配置文件 + 内存痕迹 |

#### 🛡️ 3 模式隐藏系统

| 模式 | 用途 | 技术路径 |
|------|------|----------|
| **Detection** | 仅检测不隐藏 | 默认模式，零副作用 |
| **Hide** | 对其他应用隐藏 root | eBPF 防火墙 (Android 12+) 或 mount namespace 隔离 (Android 10-11) |
| **Game** | 激进隐藏 + 性能优化 | 进程伪装 + 敏感路径屏蔽 + HWID 伪装 |

**隐藏机制**：
- **eBPF 防火墙**：拦截 `openat` / `statx` / `getdents64` / `access` 系统调用
- **Mount Namespace**：`unshare(CLONE_NEWNS)` + bind-mount 空目录覆盖敏感路径
- **LD_PRELOAD**：拦截 libc `open` / `stat` / `access` 等函数
- **白名单**：APEX-Root 自身 UID 始终可见真实状态

#### 🧬 后量子签名

- **算法**：ML-DSA-65 (CRYSTALS-Dilithium-3) — NIST 后量子签名标准
- **用途**：检测报告防篡改签名
- **实现**：liboqs 库（可选编译）

#### ⚡ 微服务架构

- **20 个独立插件** (ms001-ms020)：每个检测层对应一个可热加载的 `.so` 插件
- **YAML 配置驱动**：扫描工作流通过 `scan_workflows.yaml` 定义
- **沙箱隔离**：三副本共识机制（诱饵 / 轻隔离 / 全隔离）

#### 🎨 UI 设计

- **Liquid Glass** 风格：仿 iOS 26 液态玻璃效果
- **Compose Material3**：完全声明式 UI
- **深色/浅色主题**：自适应系统设置
- **动态流体背景**：基于 Metaball 算法的有机动画
- **可折叠顶栏**：CollapsibleGlassTopBar

---

### 📱 评估模式

| 模式 | 耗时 | 覆盖层级 | 适用场景 |
|------|------|----------|----------|
| **快速** | < 500ms | L1 / L3 / L8-L10 | 日常即时检查 |
| **标准** | 2-5s | L1-L12 + 反隐藏 | 定期安全审计 |
| **深度** | 10-30s | 全 16 层 + 侧信道 | 安全研究 |
| **取证** | 60s+ | 全层 + 自保护 + 签名 | 法医级分析 |

---

### 📊 评分算法

评分基于三个核心原则：

1. **跨层指数加权**：同时跨多个检测层的告警权重指数级高于单层告警
2. **确定性优先**：100 条"可能"级告警不如 1 条"确认"级告警
3. **相关性增强**：多个检测点的相关性大幅提升结论可信度

**风险分级**：
| 分数 | 等级 | 颜色 |
|------|------|------|
| 0-10 | ✅ 安全 | 绿色 |
| 11-30 | ⚠️ 轻度风险 | 黄色 |
| 31-60 | 🟠 中等风险 | 橙色 |
| 61-100 | ❌ 高风险 | 红色 |

---

### 🔧 技术栈

| 组件 | 技术 |
|------|------|
| **UI** | Jetpack Compose + Material3 + Liquid Glass |
| **语言** | Kotlin 1.9 + C++20 |
| **NDK** | 28.2.13676358 (ARM64) |
| **构建** | Gradle 8.2 + CMake 3.22.1 |
| **IPC** | Protobuf Lite + Unix Socket |
| **加密** | liboqs (ML-DSA-65) + SHA3-512 + AES-256-GCM |
| **eBPF** | BPF CO-RE + tracepoint |
| **架构** | MVVM + Microservice + Consensus |

---

### 📦 安装

#### 方式一：下载 APK（推荐）

1. 前往 [Releases 页面](../../releases) 下载最新 `APEX-Root-v3.1.0.apk`
2. 在设备上启用"未知来源应用安装"
3. 安装 APK
4. 授予 root 权限（需 Magisk / KernelSU / APatch）

#### 方式二：自行编译

```bash
# 1. 克隆仓库
git clone https://github.com/mengjinghao/root-check.git
cd root-check/apex-root

# 2. 配置 local.properties
echo "sdk.dir=/path/to/android-sdk" > local.properties

# 3. 编译
./gradlew assembleDebug

# 4. APK 输出
ls app/build/outputs/apk/debug/app-debug.apk
```

**编译环境要求**：
- JDK 17
- Android SDK 34 + Build Tools 34.0.0
- NDK 28.2.13676358
- CMake 3.22.1

---

### 🚀 使用指南

#### 快速开始

1. **启动应用**：首次使用需完成权限引导
2. **扫描设备**：点击仪表盘的"快速检测"按钮
3. **查看报告**：扫描完成后查看 16 层检测结果
4. **切换模式**：工具栏 → "隐藏模式" → 选择 Detection / Hide / Game

#### 功能入口

| 入口 | 功能 |
|------|------|
| 仪表盘 | 扫描 / 治愈 / 沙箱 / HWID 伪装 / 游戏模式 |
| 报告 | 详细检测结果 + 导出 |
| 警报 | 实时安全告警 |
| 设置 | 检测级别 / 隐藏策略 / 主题 / 性能 |
| 隐藏模式 | 三模式切换 + 状态监控 |
| 关于 | 版本信息 / 开源许可 / 隐私声明 |

#### Magisk 模块（可选）

项目附带独立的 `apex-hide-daemon` Magisk 模块，无需启动 app 即可在开机时自动隐藏：

1. 下载 `apex-hide-daemon.zip`
2. 在 Magisk Manager 中刷入
3. 重启设备，守护进程自动启动

---

### 📁 项目结构

```
apex-root/
├── app/                          # 主应用
│   └── src/main/
│       ├── java/com/apex/root/
│       │   ├── ui/compose/       # Compose UI (16 个 Screen)
│       │   ├── viewmodel/        # MVVM ViewModel
│       │   ├── data/             # 数据层 + JNI 桥接
│       │   ├── core/             # 核心服务
│       │   └── domain/           # 领域模型
│       ├── cpp/                  # C++ 原生代码
│       │   ├── detect/           # 16 层检测实现
│       │   ├── ctrl/             # 隐藏功能控制
│       │   ├── legacy/           # Android 10-11 回退
│       │   ├── trusted_root/     # 后量子签名
│       │   ├── micro_services/   # 20 个微服务插件
│       │   └── ebpf/             # eBPF 防火墙管理
│       └── res/                  # 资源文件
├── bpf/                          # eBPF 程序源码
├── ctrl/                         # 隐藏功能 C++ 源码
├── legacy/                       # LD_PRELOAD hook
├── jni/                          # JNI 桥接
├── module/                       # Magisk 模块
├── modules/apex-hide-daemon/     # 独立守护进程模块
├── sepolicy/                     # SELinux 策略
└── scripts/                      # 构建脚本
```

---

### 🔒 隐私声明

- **完全本地运行**：不上传任何设备信息到云端
- **检测结果仅存储在设备本地**
- **报告导出由用户主动触发**，不含敏感标识符
- **后量子签名密钥每次生成**，不持久化
- **不收集、不分享任何使用统计或崩溃日志**

---

### ⚖️ 免责声明

本应用仅供安全研究与设备完整性评估使用。使用本应用进行的任何操作（包括但不限于 root 隐藏、系统治愈、硬件伪装）由用户自行承担风险。开发者不对因使用本应用导致的任何设备损坏、数据丢失或违反服务条款负责。请遵守当地法律法规。

---

### 📜 开源许可

本应用使用以下开源组件：
- Jetpack Compose (Apache 2.0)
- Material 3 Components (Apache 2.0)
- liboqs (MIT)
- AndroidX (Apache 2.0)
- Kotlin Coroutines (Apache 2.0)

---

### 👤 开发者

| | |
|---|---|
| **开发者** | MJH |
| **QQ** | 2544240258 |
| **WeChat** | meng4117222 |

> 💬 如有问题或建议，欢迎通过 QQ / 微信联系开发者。

---

### 📈 版本历史

| 版本 | 日期 | 主要变更 |
|------|------|----------|
| v3.1.0 | 2026-06 | 16 层检测 + 3 模式隐藏 + 后量子签名 + UI 重构 |
| v3.0.0 | 2026-05 | Ring0→Ring3 迁移 + 微服务架构 |
| v2.0.0 | 2026-03 | Compose UI 重写 + Liquid Glass |
| v1.0.0 | 2026-01 | 初始版本 |

---

<div align="center">

**⭐ 如果本项目对您有帮助，请给个 Star ⭐**

</div>

---

## English Document

### 📖 Overview

**APEX-Root** is a professional-grade security analysis tool for Android device integrity assessment. It employs a **16-layer deep detection architecture** covering full-dimension security auditing from system properties to firmware integrity, combined with a **3-mode hiding system** (Detection/Hide/Game) for transparent root management.

Core philosophy: **Bridging the chasm between perceived security and ground truth** — providing a diagnostic instrument of uncompromising precision for security researchers, developers, and enterprise IT administrators.

---

### ✨ Key Features

#### 🔍 16-Layer Detection Architecture

| Layer | Name | Detection Target | Implementation |
|-------|------|------------------|----------------|
| **L1** | System Properties | `ro.debuggable` / `ro.secure` / `ro.build.tags` | Raw `/dev/__properties__` read |
| **L2** | ART Injection | Frida / Xposed / LSPosed / SandHook / Pine | `/proc/self/maps` scan |
| **L3** | Memory Fingerprint | Magisk / Zygisk / Shamiko / Riru anon exec | RWX page + signature match |
| **L4** | Mount Check | overlayfs / bind-mount / namespace isolation | `/proc/self/mountinfo` parse |
| **L5** | Side-Channel | Syscall timing + result consistency | ARM64 raw `svc` timing |
| **L6** | Root Daemon | magiskd / ksud / apd / sukid / kitana scan | `/proc/*/cmdline` enum |
| **L7** | Boot Chain | Bootloader lock / AVB / dm-verity / vbmeta | `/proc/cmdline` + sysfs |
| **L8** | Magisk | Mainstream + Delta/Kitsune/Kitana forks + DenyList | Daemon + file paths |
| **L9** | KernelSU | KSU / SukiSU / KSU-NEXT + Manager APP | Main dir + package names |
| **L10** | APatch | APD / KPM user-space modules + Manager APP | `/data/adb/ap/` paths |
| **L11** | Hook Framework | Xposed / LSPosed / Frida / Substrate / Epic | Memory maps + binary paths |
| **L12** | Custom ROM | 50+ ROM signatures (LineageOS / ArrowOS / Rising) | build.prop multi-partition |
| **L13** | Firmware Integrity | TEE / Modem / Recovery / Vendor partitions | sysfs + mount attributes |
| **L14** | Virtual Framework | VirtualXposed / TaiChi / Dual Space / Island | Package + process scan |
| **L15** | Dangerous Apps | GameGuardian / CheatEngine / Lucky Patcher | `/data/data` path detection |
| **L16** | Magisk Extensions | DenyList / ZygiskNext / ReZygisk / LSPosed / Riru | Config + memory traces |

#### 🛡️ 3-Mode Hiding System

| Mode | Purpose | Technical Path |
|------|---------|----------------|
| **Detection** | Detect only, no hiding | Default mode, zero side effects |
| **Hide** | Hide root from other apps | eBPF firewall (Android 12+) or mount namespace isolation (Android 10-11) |
| **Game** | Aggressive hiding + performance | Process disguise + sensitive path masking + HWID spoof |

**Hiding Mechanisms**:
- **eBPF Firewall**: Intercepts `openat` / `statx` / `getdents64` / `access` syscalls
- **Mount Namespace**: `unshare(CLONE_NEWNS)` + bind-mount empty dirs over sensitive paths
- **LD_PRELOAD**: Intercepts libc `open` / `stat` / `access` functions
- **Whitelist**: APEX-Root's own UID always sees real status

#### 🧬 Post-Quantum Signing

- **Algorithm**: ML-DSA-65 (CRYSTALS-Dilithium-3) — NIST post-quantum standard
- **Purpose**: Tamper-proof detection report signing
- **Implementation**: liboqs library (optional)

#### ⚡ Microservice Architecture

- **20 independent plugins** (ms001-ms020): Each detection layer maps to a hot-loadable `.so` plugin
- **YAML configuration**: Scan workflows defined via `scan_workflows.yaml`
- **Sandbox isolation**: Three-replica consensus (bait / light isolation / full isolation)

#### 🎨 UI Design

- **Liquid Glass** style: iOS 26-inspired liquid glass effect
- **Compose Material3**: Fully declarative UI
- **Dark/Light theme**: System adaptive
- **Dynamic fluid background**: Metaball-based organic animation
- **Collapsible top bar**: CollapsibleGlassTopBar

---

### 📱 Assessment Modes

| Mode | Duration | Coverage | Use Case |
|------|----------|----------|----------|
| **Quick** | < 500ms | L1 / L3 / L8-L10 | Daily instant check |
| **Standard** | 2-5s | L1-L12 + anti-hiding | Regular security audit |
| **Deep** | 10-30s | All 16 layers + side-channel | Security research |
| **Forensic** | 60s+ | All + self-protection + signing | Forensic analysis |

---

### 📊 Scoring Algorithm

Scoring is based on three core principles:

1. **Cross-layer exponential weighting**: Alerts spanning multiple layers have exponentially higher weight
2. **Certainty priority**: 100 "possible" alerts < 1 "confirmed" alert
3. **Correlation enhancement**: Correlation between detection points greatly boosts credibility

**Risk Levels**:
| Score | Level | Color |
|-------|-------|-------|
| 0-10 | ✅ Safe | Green |
| 11-30 | ⚠️ Low Risk | Yellow |
| 31-60 | 🟠 Medium Risk | Orange |
| 61-100 | ❌ High Risk | Red |

---

### 🔧 Tech Stack

| Component | Technology |
|-----------|------------|
| **UI** | Jetpack Compose + Material3 + Liquid Glass |
| **Language** | Kotlin 1.9 + C++20 |
| **NDK** | 28.2.13676358 (ARM64) |
| **Build** | Gradle 8.2 + CMake 3.22.1 |
| **IPC** | Protobuf Lite + Unix Socket |
| **Crypto** | liboqs (ML-DSA-65) + SHA3-512 + AES-256-GCM |
| **eBPF** | BPF CO-RE + tracepoint |
| **Architecture** | MVVM + Microservice + Consensus |

---

### 📦 Installation

#### Option 1: Download APK (Recommended)

1. Go to [Releases page](../../releases) and download `APEX-Root-v3.1.0.apk`
2. Enable "Install unknown apps" on your device
3. Install the APK
4. Grant root access (requires Magisk / KernelSU / APatch)

#### Option 2: Build from Source

```bash
# 1. Clone repository
git clone https://github.com/mengjinghao/root-check.git
cd root-check/apex-root

# 2. Configure local.properties
echo "sdk.dir=/path/to/android-sdk" > local.properties

# 3. Build
./gradlew assembleDebug

# 4. APK output
ls app/build/outputs/apk/debug/app-debug.apk
```

**Build Requirements**:
- JDK 17
- Android SDK 34 + Build Tools 34.0.0
- NDK 28.2.13676358
- CMake 3.22.1

---

### 🚀 Usage Guide

#### Quick Start

1. **Launch app**: Complete permission guide on first use
2. **Scan device**: Tap "Quick Scan" on dashboard
3. **View report**: Check 16-layer detection results
4. **Switch mode**: Toolbar → "Hide Mode" → Select Detection / Hide / Game

#### Feature Entry Points

| Entry | Function |
|-------|----------|
| Dashboard | Scan / Cure / Sandbox / HWID spoof / Game mode |
| Report | Detailed results + export |
| Alert | Real-time security alerts |
| Settings | Detection level / Hide strategy / Theme / Performance |
| Hide Mode | 3-mode switch + status monitor |
| About | Version / Open source licenses / Privacy |

#### Magisk Module (Optional)

The project includes a standalone `apex-hide-daemon` Magisk module for boot-time hiding without launching the app:

1. Download `apex-hide-daemon.zip`
2. Flash in Magisk Manager
3. Reboot, daemon auto-starts

---

### 📁 Project Structure

```
apex-root/
├── app/                          # Main application
│   └── src/main/
│       ├── java/com/apex/root/
│       │   ├── ui/compose/       # Compose UI (16 Screens)
│       │   ├── viewmodel/        # MVVM ViewModel
│       │   ├── data/             # Data layer + JNI bridge
│       │   ├── core/             # Core services
│       │   └── domain/           # Domain models
│       ├── cpp/                  # C++ native code
│       │   ├── detect/           # 16-layer detection
│       │   ├── ctrl/             # Hide functionality
│       │   ├── legacy/           # Android 10-11 fallback
│       │   ├── trusted_root/     # Post-quantum signing
│       │   ├── micro_services/   # 20 microservice plugins
│       │   └── ebpf/             # eBPF firewall management
│       └── res/                  # Resources
├── bpf/                          # eBPF program source
├── ctrl/                         # Hide functionality C++
├── legacy/                       # LD_PRELOAD hook
├── jni/                          # JNI bridge
├── module/                       # Magisk module
├── modules/apex-hide-daemon/     # Standalone daemon module
├── sepolicy/                     # SELinux policy
└── scripts/                      # Build scripts
```

---

### 🔒 Privacy Statement

- **Fully local**: No device information uploaded to cloud
- **Results stored locally only**
- **Export triggered by user**, no sensitive identifiers
- **Post-quantum keys generated per-use**, not persisted
- **No usage statistics or crash logs collected**

---

### ⚖️ Disclaimer

This application is for security research and device integrity assessment only. Users assume all risks for any operations performed (including but not limited to root hiding, system curing, hardware spoofing). The developer is not liable for any device damage, data loss, or Terms of Service violations resulting from the use of this application. Please comply with local laws and regulations.

---

### 📜 Open Source Licenses

This application uses the following open-source components:
- Jetpack Compose (Apache 2.0)
- Material 3 Components (Apache 2.0)
- liboqs (MIT)
- AndroidX (Apache 2.0)
- Kotlin Coroutines (Apache 2.0)

---

### 👤 Developer

| | |
|---|---|
| **Developer** | MJH |
| **QQ** | 2544240258 |
| **WeChat** | meng4117222 |

> 💬 For questions or suggestions, feel free to contact via QQ / WeChat.

---

### 📈 Version History

| Version | Date | Major Changes |
|---------|------|---------------|
| v3.1.0 | 2026-06 | 16-layer detection + 3-mode hiding + post-quantum signing + UI overhaul |
| v3.0.0 | 2026-05 | Ring0→Ring3 migration + microservice architecture |
| v2.0.0 | 2026-03 | Compose UI rewrite + Liquid Glass |
| v1.0.0 | 2026-01 | Initial release |

---

<div align="center">

**⭐ If this project helps you, please give it a Star ⭐**

</div>
