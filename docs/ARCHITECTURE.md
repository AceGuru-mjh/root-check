# 架构设计文档

本文档介绍 APEX-Root 的整体架构、核心模块设计和技术选型。

---

## 📐 整体架构

```
┌─────────────────────────────────────────────────────────┐
│                    UI 层 (Jetpack Compose)                │
│  DashboardScreen │ ReportScreen │ HideModeScreen │ ...  │
└─────────────────────────┬───────────────────────────────┘
                          │ StateFlow collectAsState
┌─────────────────────────┴───────────────────────────────┐
│                  ViewModel 层 (MVVM)                      │
│  ApexViewModel │ ScanViewModel │ SettingsViewModel │     │
│  UpdateViewModel                                        │
└─────────────────────────┬───────────────────────────────┘
                          │ viewModelScope.launch(IO)
┌─────────────────────────┴───────────────────────────────┐
│                  Data 层 (Repository)                     │
│  RootDetectRepositoryImpl │ SettingsRepository │         │
│  DetectionCache │ FingerprintDatabase │ AppUpdater      │
└─────────────────────────┬───────────────────────────────┘
                          │ NativeLibraryLoader.safeCall
┌─────────────────────────┴───────────────────────────────┐
│              Native 层 (C++ / JNI / NDK)                  │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐   │
│  │ detect/  │ │ ctrl/    │ │ ebpf/    │ │ cure/    │   │
│  │ 16 层检测│ │ 隐藏控制 │ │ eBPF 防火│ │ 修复引擎│   │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘   │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐   │
│  │ guard/   │ │ game/    │ │ hid/     │ │ consensus│   │
│  │ 实时防护 │ │ 游戏模式 │ │ HWID伪装 │ │ 三副本   │   │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘   │
└─────────────────────────────────────────────────────────┘
```

---

## 🔍 16 层检测架构

APEX-Root 采用 16 层深度检测，每层对应一个独立的 C++ 模块：

| 层级 | 名称 | 检测目标 | 实现文件 |
|------|------|----------|----------|
| L1 | 系统属性 | `ro.debuggable` / `ro.secure` / `ro.build.tags` | `layer1_prop.cpp` |
| L2 | ART 注入 | Frida / Xposed / LSPosed / SandHook / Pine | `layer2_art.cpp` |
| L3 | 内存特征 | Magisk / Zygisk / Shamiko / Riru 匿名可执行映射 | `layer3_mem.cpp` |
| L4 | 挂载检查 | overlayfs / bind-mount / 命名空间隔离 | `layer4_mount.cpp` |
| L5 | 侧信道 | syscall 时延分析 + 结果一致性检测 | `layer5_sidechannel.cpp` |
| L6 | Root 守护 | magiskd / ksud / apd / sukid / kitana 进程扫描 | `layer6_kernel.cpp` |
| L7 | 启动链 | Bootloader 锁 / AVB / dm-verity / vbmeta | `layer7_boot.cpp` |
| L8 | Magisk | 主流 + Delta/Kitsune/Kitana fork + DenyList | `layer8_magisk.cpp` |
| L9 | KernelSU | KSU / SukiSU / KSU-NEXT + Manager APP | `layer9_ksu.cpp` |
| L10 | APatch | APD / KPM 用户态模块 + Manager APP | `layer10_apatch.cpp` |
| L11 | Hook 框架 | Xposed / LSPosed / Frida / Substrate / Epic | `layer11_hook.cpp` |
| L12 | 自定义 ROM | 50+ ROM 标识 (LineageOS / ArrowOS / Rising 等) | `layer12_rom.cpp` |
| L13 | 固件完整性 | TEE / Modem / Recovery / Vendor 分区 | `layer13_firmware.cpp` |
| L14 | 虚拟框架 | VirtualXposed / 太极 / 双开分身 / Island | `layer14_virtualxposed.cpp` |
| L15 | 危险应用 | GameGuardian / CheatEngine / Lucky Patcher | `layer15_dangerous_apps.cpp` |
| L16 | Magisk 扩展 | DenyList / ZygiskNext / ReZygisk / LSPosed / Riru | `layer16_magisk_extensions.cpp` |

### 微服务架构

每层对应一个可热加载的 `.so` 插件（`ms001` - `ms020`）：

```
app/src/main/cpp/micro_services/
├── engine/
│   ├── service_engine.cpp     # 微服务引擎
│   └── yaml_parser.cpp        # 工作流 YAML 解析
├── sandbox/
│   ├── sandbox_isolator.cpp   # 沙箱隔离
│   └── sandbox_main.cpp
└── services/
    ├── ms001_file_scanner/     # L1 插件
    ├── ms002_proc_scanner/     # L2 插件
    ├── ...
    └── ms020_sandbox_detect/   # L20 插件
```

扫描工作流通过 `assets/workflows/scan_workflows.yaml` 定义：

```yaml
quick_scan:
  - ms001_file_scanner
  - ms003_mem_scanner
  - ms008_bootloader
  - ms009_custom_rom
  - ms010_dangerous_apps

deep_scan:
  - ms001_file_scanner
  - ms002_proc_scanner
  - ms003_mem_scanner
  - ... # 全部 20 个微服务
```

---

## 🛡️ 3 模式隐藏系统

| 模式 | 用途 | 技术路径 |
|------|------|----------|
| **Detection** | 仅检测不隐藏 | 默认模式，零副作用 |
| **Hide** | 对其他应用隐藏 root | eBPF 防火墙 (Android 12+) 或 mount namespace 隔离 (Android 10-11) |
| **Game** | 激进隐藏 + 性能优化 | 进程伪装 + 敏感路径屏蔽 + HWID 伪装 |

### 隐藏机制

```
┌─────────────────────────────────────┐
│          用户切换到 Hide 模式         │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│    HideModeManager.switchToMode()   │
│    → NativeBridge.enableHideMode()  │
└──────────────┬──────────────────────┘
               │
       ┌───────┴───────┐
       ▼               ▼
┌─────────────┐  ┌─────────────┐
│ Android 12+ │  │ Android 10-11│
│ eBPF 路径   │  │ Legacy 路径 │
└──────┬──────┘  └──────┬──────┘
       │                │
       ▼                ▼
┌─────────────┐  ┌─────────────┐
│ eBPF 拦截   │  │ mount ns    │
│ openat/     │  │ + LD_PRELOAD│
│ statx/      │  │ + bind-mount│
│ getdents64  │  │             │
└─────────────┘  └─────────────┘
```

**eBPF 路径（Android 12+ GKI 5.10+）**：
- tracepoint 拦截 `openat` / `statx` / `getdents64` / `access`
- 白名单模式：仅 APEX UID 绕过
- 纯净基线数据存储在 BPF_MAP_TYPE_ARRAY

**Legacy 路径（Android 10-11）**：
- `unshare(CLONE_NEWNS)` 命名空间隔离
- 敏感路径 bind-mount 为空目录
- LD_PRELOAD `libapex_hook.so` 拦截 libc 函数

---

## 🧬 后量子签名

APEX-Root 使用 ML-DSA-65 (CRYSTALS-Dilithium-3) 对检测报告签名，防止篡改：

```
检测完成 → 生成报告 JSON
          ↓
     SHA3-512 哈希
          ↓
   ML-DSA-65 签名（每次生成临时密钥对）
          ↓
   报告 + 签名 + 公钥 → 可验证
```

**实现**：
- 算法：ML-DSA-65 (NIST 后量子签名标准)
- 库：liboqs（可选编译，`APEX_USE_LIBOQS=ON`）
- 密钥：每次生成，不持久化
- 验证：接收方用公钥验证签名

---

## 🔌 IPC 通信

APEX-Root 使用 Unix Socket + Protobuf 进行进程间通信（沙箱 ↔ 主进程）：

```
┌──────────────┐     Unix Socket     ┌──────────────┐
│  主进程       │ ←─────────────────→ │  沙箱进程     │
│  (APEX-Root) │   Protobuf 消息     │  (隔离环境)   │
└──────────────┘                     └──────────────┘
```

**协议**：
- `detection.proto`：定义 ScanTask / GlobalSecureReport / SecurityAlert
- `DetectionProtocol`：消息编解码（magic byte + payload）
- `SecureSocketClient`：Unix Socket 客户端

---

## 📱 软件更新架构

```
┌─────────────────────────────────────┐
│           UpdateScreen (UI)          │
└──────────────┬──────────────────────┘
               │ collectAsState
┌──────────────┴──────────────────────┐
│         UpdateViewModel              │
│  - uiState: UpdateUiState           │
│  - downloadState: DownloadState     │
│  - moduleCheckState: ModuleCheckState│
└──────────────┬──────────────────────┘
               │
┌──────────────┴──────────────────────┐
│           AppUpdater                 │
│  - GitHub Releases API              │
│  - HttpURLConnection 流式下载       │
│  - FileProvider 安装                │
└─────────────────────────────────────┘
```

**更新流程**：
1. `checkForUpdates()` → 调用 GitHub API `/releases/latest`
2. 比较版本号（semantic versioning）
3. 用户点击下载 → 流式下载 APK 到 cacheDir
4. 下载完成 → `FileProvider + ACTION_VIEW` 触发系统安装器

---

## 📊 评分算法

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

## 📁 项目结构

```
apex-root/
├── app/                              # 主应用
│   └── src/main/
│       ├── java/com/apex/root/
│       │   ├── ui/compose/           # Compose UI (16+ Screen)
│       │   ├── viewmodel/            # MVVM ViewModel
│       │   ├── data/                 # 数据层 + JNI 桥接
│       │   │   ├── jni/NativeBridge.kt
│       │   │   ├── repository/
│       │   │   ├── updater/          # 软件更新
│       │   │   └── ...
│       │   ├── core/                 # 核心服务
│       │   │   ├── NativeLibraryLoader.kt
│       │   │   ├── permission/
│       │   │   ├── security/
│       │   │   └── notification/
│       │   ├── domain/               # 领域模型
│       │   ├── island/               # 沙箱
│       │   ├── game/                 # 游戏模式
│       │   ├── guard/                # 实时防护
│       │   ├── hid/                  # HWID 伪装
│       │   └── cure/                 # 修复
│       ├── cpp/                      # C++ 原生代码
│       │   ├── detect/               # 16 层检测
│       │   ├── ctrl/                 # 隐藏控制
│       │   ├── ebpf/                 # eBPF 防火墙
│       │   ├── cure/                 # 修复引擎
│       │   ├── guard/                # 实时防护
│       │   ├── game/                 # 游戏模式
│       │   ├── hid/                  # HWID 伪装
│       │   ├── micro_services/       # 20 个微服务插件
│       │   ├── trusted_root/         # 后量子签名
│       │   ├── consensus/            # 三副本共识
│       │   └── bare_syscall/         # 裸 syscall
│       ├── res/                      # 资源文件
│       └── AndroidManifest.xml
├── bpf/                              # eBPF 程序源码
├── ctrl/                             # 隐藏功能 C++ 源码
├── legacy/                           # LD_PRELOAD hook
├── jni/                              # JNI 桥接
├── module/                           # Magisk 模块
├── modules/apex-hide-daemon/         # 独立守护进程模块
├── sepolicy/                         # SELinux 策略
├── scripts/                          # 构建脚本
└── docs/                             # 文档
```

---

## 🔧 技术栈

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

## 📚 相关文档

- [环境搭建](./BUILD_ENV.md)
- [构建指南](./BUILD.md)
- [故障排查](./TROUBLESHOOTING.md)
- [开发指南](./DEVELOPMENT.md)
