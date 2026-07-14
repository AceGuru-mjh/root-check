# APEX Root Detection System - 知识图谱

## 📋 项目概述

**APEX Root** 是一个企业级 Android Root 检测系统，采用分层架构设计，提供从基础系统属性检查到深度内核分析的多层次检测能力。

**当前版本**: 2.0.0-m3 (Material Design 3 重构版)  
**目标平台**: Android 8.0+ (API 26+)  
**开发语言**: Kotlin (应用层) + C++ (Native 层)

---

## 🏗️ 架构总览

```
┌─────────────────────────────────────────────────────────────┐
│                      UI Layer (Compose M3)                   │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │
│  │ Dashboard    │  │ Settings     │  │ ScanResult   │       │
│  │ Screen       │  │ Screen       │  │ Screen       │       │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘       │
│         │                 │                 │                │
│  ┌──────▼───────┐  ┌──────▼───────┐  ┌──────▼───────┐       │
│  │ DashboardVM  │  │ SettingsVM   │  │ ResultVM     │       │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘       │
└─────────┼─────────────────┼─────────────────┼────────────────┘
          │                 │                 │
┌─────────▼─────────────────▼─────────────────▼────────────────┐
│                    Domain Layer (Models)                      │
│  DeviceInfo | RootDetectionResult | ScanSession | GuardEvent │
└─────────┬─────────────────┬─────────────────┬────────────────┘
          │                 │                 │
┌─────────▼─────────────────▼─────────────────▼────────────────┐
│                    Data Layer (Repository)                    │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐      │
│  │ DeviceRepo  │  │ ScanRepo    │  │ GuardEventRepo  │      │
│  └──────┬──────┘  └──────┬──────┘  └────────┬────────┘      │
└─────────┼─────────────────┼──────────────────┼───────────────┘
          │                 │                  │
┌─────────▼─────────────────▼──────────────────▼───────────────┐
│                    Native Bridge (JNI)                        │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐      │
│  │ libbridge   │◄─┤  libcore    │◄─┤ libdetection    │      │
│  └─────────────┘  └─────────────┘  └─────────────────┘      │
│                        │                                     │
│         ┌──────────────┴──────────────┐                     │
│         ▼                             ▼                     │
│  ┌─────────────┐              ┌─────────────┐               │
│  │ L1-L12      │              │ Utils       │               │
│  │ Detectors   │              │ Helpers     │               │
│  └─────────────┘              └─────────────┘               │
└─────────────────────────────────────────────────────────────┘
```

---

## 📦 核心模块详解

### 1. UI Layer (Jetpack Compose M3)

#### 组件库 (`ui/components/`)
| 组件 | 功能 | 状态 |
|------|------|------|
| `ApexTopAppBar` | 可滚动顶部栏，支持副标题 | ✅ |
| `ApexCard` | 12dp 圆角卡片容器 | ✅ |
| `ApexButton` | Material 3 标准按钮 | ✅ |
| `ApexListItem` | 列表项 (图标 + 标题 + 副标题) | ✅ |

#### 页面 (`ui/screens/`)
| 页面 | 功能 | ViewModel | 状态 |
|------|------|-----------|------|
| `DashboardScreen` | 仪表盘：设备摘要/快速扫描/风险概览 | `DashboardViewModel` | ✅ |
| `SettingsScreen` | 设置：常规/检测层/高级/关于 | `SettingsViewModel` | 🔄 |

#### 主题 (`ui/theme/`)
- **Color.kt**: 浅色/深色主题 + 风险等级颜色 (绿/黄/橙/红/紫)
- **Typography.kt**: Display/Large/Medium/Small/Label 完整字体排印
- **Shape.kt**: ExtraSmall(4dp) → Large(16dp) 圆角系统
- **Theme.kt**: 动态色彩支持 (Android 12+)

---

### 2. Domain Layer (`domain/model/`)

#### 核心数据类

```kotlin
data class DeviceInfo(
    val brand: String,           // 品牌 (Build.BRAND)
    val model: String,           // 型号 (Build.MODEL)
    val androidVersion: String,  // Android 版本
    val sdkInt: Int,             // API 级别
    val securityPatchLevel: String,  // 安全补丁
    val bootloaderStatus: BootloaderStatus,  // UNLOCKED/LOCKED/UNKNOWN
    val selinuxMode: SELinuxMode           // ENFORCING/PERMISSIVE/DISABLED
)

data class RootDetectionResult(
    val isRooted: Boolean,       // 是否 Root
    val confidence: Float,       // 置信度 (0.0-1.0)
    val detectedMethods: List<DetectionMethod>,  // 检测方法列表
    val riskLevel: RiskLevel,    // NONE/LOW/MEDIUM/HIGH/CRITICAL
    val timestamp: Long          // 时间戳
)

data class ScanSession(
    val sessionId: String,
    val startTime: Long,
    val endTime: Long?,
    val progress: Float,         // 0.0-1.0
    val currentLayer: DetectionLayer?,
    val result: RootDetectionResult?,
    val status: ScanStatus       // IDLE/RUNNING/PAUSED/COMPLETED/ERROR
)

data class GuardEvent(
    val eventId: String,
    val type: GuardEventType,    // ROOT_ATTEMPT/SUSPICIOUS_APP/...
    val severity: SeverityLevel, // INFO/WARNING/ALERT/CRITICAL
    val description: String,
    val packageName: String?,
    val timestamp: Long,
    val handled: Boolean
)
```

#### 枚举类型

```kotlin
enum class RiskLevel { NONE, LOW, MEDIUM, HIGH, CRITICAL }
enum class DetectionLayer { L1_PROP, L4_BINARY, L6_NATIVE, L8_BEHAVIOR, L9_ADVANCED, L10_AI, L12_DEEP }
enum class ScanStatus { IDLE, RUNNING, PAUSED, COMPLETED, ERROR }
enum class BootloaderStatus { UNLOCKED, LOCKED, UNKNOWN }
enum class SELinuxMode { ENFORCING, PERMISSIVE, DISABLED, UNKNOWN }
enum class GuardEventType { ROOT_ATTEMPT, SUSPICIOUS_APP, SYSTEM_MODIFICATION, BOOTLOADER_CHANGE }
enum class SeverityLevel { INFO, WARNING, ALERT, CRITICAL }
```

---

### 3. Data Layer (`data/repository/`)

#### DeviceRepository
```kotlin
class DeviceRepository {
    suspend fun getDeviceInfo(): DeviceInfo      // 带缓存
    suspend fun refreshDeviceInfo(): DeviceInfo  // 刷新
}
```
- 从 Android Build 类获取设备信息
- SELinux 模式通过 `getenforce` 命令检测
- Bootloader 状态待 Native 层实现

#### TODO Repositories
- `ScanRepository`: Root 检测执行 (调用 Native 层)
- `GuardEventRepository`: 事件存储 (Room 数据库)
- `SettingsRepository`: 用户偏好 (DataStore)

---

### 4. Native Layer (`jni/`)

#### 目录结构
```
jni/
├── bridge/        # JNI 桥接层 (Java↔C++)
├── core/          # 核心工具函数
├── detection/     # 12 层检测实现
│   ├── layer1_prop.cpp      # 系统属性检查
│   ├── layer4_binary.cpp    # 二进制文件检测
│   ├── layer6_native.cpp    # Native syscall 测试
│   ├── layer8_behavior.cpp  # 行为分析
│   ├── layer9_advanced.cpp  # 高级启发式
│   ├── layer10_ai.cpp       # ML 检测
│   └── layer12_deep.cpp     # 深度内核检查
└── utils/         # 通用工具
    ├── file_utils.cpp
    ├── prop_utils.cpp
    └── logger.cpp
```

#### 检测层说明
| 层级 | 检测内容 | 复杂度 |
|------|----------|--------|
| L1 | build.prop 属性检查 | ⭐ |
| L4 | su/binary/superuser.apk 文件存在性 | ⭐⭐ |
| L6 | Native syscall 测试 (fork, ptrace) | ⭐⭐⭐ |
| L8 | 应用行为分析 (反射、动态加载) | ⭐⭐⭐⭐ |
| L9 | 高级启发式 (时序分析、环境指纹) | ⭐⭐⭐⭐⭐ |
| L10 | 机器学习模型推理 | ⭐⭐⭐⭐⭐ |
| L12 | 内核模块检查、VMProtect 检测 | ⭐⭐⭐⭐⭐⭐ |

---

## 🔄 数据流

### 扫描流程
```
用户点击"开始扫描"
    ↓
DashboardViewModel.startScan()
    ↓
更新 UI 状态 (isScanning=true, progress=0)
    ↓
ScanRepository.executeScan()
    ↓
NativeBridge.isDeviceRooted()
    ↓
并行执行 L1→L12 检测层
    ↓
汇总结果 → RootDetectionResult
    ↓
更新 UI 状态 (isScanning=false, lastScanResult=...)
    ↓
显示风险等级和检测方法
```

### 设备信息加载流程
```
ViewModel init
    ↓
DeviceRepository.getDeviceInfo()
    ↓
检查缓存 → 有则返回
    ↓
无缓存则调用 Build.* 和系统命令
    ↓
创建 DeviceInfo 对象并缓存
    ↓
更新 UI 状态
```

---

## 🎯 关键设计决策

### 1. MVVM + Clean Architecture
- **UI 层**: 纯展示逻辑，无业务代码
- **ViewModel**: 持有状态，协调 View 和 Model
- **Repository**: 统一数据源接口，隐藏实现细节
- **Domain**: 纯 Kotlin 数据类，无 Android 依赖

### 2. StateFlow 响应式状态管理
```kotlin
private val _uiState = MutableStateFlow(DashboardUiState())
val uiState: StateFlow<DashboardUiState> = _uiState.asStateFlow()
```
- 单方向数据流
- 自动 UI 更新
- 线程安全

### 3. LazyColumn 性能优化
- 替代 verticalScroll 解决长列表卡顿
- 支持 200+ 配置项流畅滚动
- 自动回收不可见项

### 4. 分层检测架构
- 7 层独立检测，可单独启用/禁用
- 权重累加计算最终风险等级
- 支持插件化扩展新检测层

---

## ⚠️ 已知问题 (待修复)

### P0 - 严重
1. ~~DetectionCache 线程安全问题~~ (已在新架构中修复)
2. ~~ViewModel 资源泄漏~~ (已改用 proper coroutine scope)

### P1 - 高优先级
1. Native 层尚未集成 (当前使用 Mock 数据)
2. SettingsViewModel 未实现
3. 事件历史记录未实现

### P2 - 中优先级
1. 无障碍支持不完整
2. 缺少单元测试
3. 国际化覆盖不全

---

## 📅 开发路线图

### Phase 0: 基础设施 ✅
- [x] M3 主题系统
- [x] 基础组件库
- [x] 导航框架
- [x] 领域模型

### Phase 1: ViewModel 与数据层 🔄
- [x] DashboardViewModel
- [x] DeviceRepository
- [ ] SettingsViewModel
- [ ] ScanRepository (Native 桥接)
- [ ] GuardEventRepository

### Phase 2: 功能完善
- [ ] 真实 Root 检测集成
- [ ] 扫描进度实时更新
- [ ] 事件历史记录
- [ ] 通知系统

### Phase 3: 优化与测试
- [ ] 性能优化
- [ ] 无障碍支持
- [ ] 单元测试
- [ ] UI 测试

### Phase 4: 文档与发布
- [ ] 用户手册
- [ ] 开发者文档
- [ ] 发布准备

---

## 🔧 开发环境

- **Android Studio**: Hedgehog (2023.1.1) 或更高
- **Gradle**: 8.5
- **Kotlin**: 1.9.21
- **Compose BOM**: 2024.01.00
- **NDK**: 25.2 (for Native layer)
- **minSdk**: 26 (Android 8.0)
- **targetSdk**: 34 (Android 14)

---

## 📚 相关文档

- [UI_REFACTOR_SPEC.md](UI_REFACTOR_SPEC.md) - UI 重构详细规范
- [PROGRESS_LOG.md](PROGRESS_LOG.md) - 开发进度日志
- [ARCHITECTURE.md](ARCHITECTURE.md) - 架构设计文档 (TODO)
- [NATIVE_GUIDE.md](NATIVE_GUIDE.md) - Native 层开发指南 (TODO)

---

**最后更新**: 2024-01-XX  
**维护者**: APEX Team
