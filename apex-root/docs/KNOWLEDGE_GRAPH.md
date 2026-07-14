# APEX Root 知识图谱

> 本文档用于持续开发过程中快速理解项目架构、数据流和模块关系。每次完成一小部分开发后需更新此图谱。

**最后更新时间**: 2026-07-14 (v1.0.4 M3 UI 重构完成)  
**当前阶段**: M3 UI Phase 4 完成 — Dashboard + Settings 可用,待完善其余页面

---

## 一、核心领域模型 (Domain Layer)

### 1.1 检测结果模型
```
ScanResult
├── isRooted: Boolean
├── riskLevel: RiskLevel (SAFE/WARNING/DANGER/CRITICAL)
├── details: String
├── riskScore: Int (0-100)
└── timestamp: Long

RootType
├── NONE(0), MAGISK(1), KERNELSU(2), APATCH(3), UNKNOWN(4)

CureResult
├── success: Boolean
├── rootType: RootType
├── levelUsed: CureLevel (LIGHT/STANDARD/DEEP/FACTORY)
├── message: String
└── needsReboot: Boolean
```

### 1.2 信任层模型 (Trust Model)
```
TrustedLayerResult
├── serviceId: String
├── serviceName: String
├── success: Boolean
├── confidence: Float (0-1)
├── findings: List<Finding>
├── rawHash: ByteArray
├── signatures: List<ByteArray>
└── durationMs: Long

Finding
├── type: FindingType (14 种类型)
├── severity: Severity (SAFE/INFO/LOW/MEDIUM/HIGH/CRITICAL)
├── description: String
└── evidence: String

FindingType 枚举:
ROOT_BINARY, SU_HIDE, PROPERTY_TAMPER, HOOK_DETECTED,
MOUNT_OVERLAY, KERNEL_ROOTKIT, TEE_TAMPER, BOOTLOADER_UNLOCKED,
CUSTOM_ROM, DANGEROUS_APP, PIF_DETECTED, ZYGISK_INJECTION,
VIRTUALIZATION, MEMORY_TAMPER, PROCESS_HIDE

GlobalSecureReport
├── reportId: String
├── timestamp: Long
├── taskId: String
├── results: List<TrustedLayerResult>
├── consensusSignatures: List<ByteArray>
├── overallRisk: Severity
├── riskScore: Float
└── daemonSignature: ByteArray

SecurityAlert
├── alertId: String
├── type: AlertType (7 种类型)
├── severity: Severity
├── description: String
├── sourceReplica: String
├── timestamp: Long
└── evidence: ByteArray

ScanTask
├── taskId: String
├── level: DetectionLevel (QUICK/STANDARD/DEEP/FORENSIC)
├── enabledServices: List<String>
├── nonce: ByteArray (32 bytes)
└── timestamp: Long
```

### 1.3 防护状态模型 (Guard Model)
```
GuardState
├── enabled: Boolean
├── systemIntegrityOk: Boolean
└── alertCount: Int
```

### 1.4 并行检测模型
```
LayerResult
├── layerId: Int (1-20)
├── layerName: String
├── detected: Boolean
├── weight: Int (5-12)
├── latencyMs: Long
└── detail: String

ParallelScanResult
├── layers: List<LayerResult>
├── totalRiskScore: Int (0-100)
├── totalLatencyMs: Long
├── detectedCount: Int
├── totalLayers: Int (19)
└── timestamp: Long
```

---

## 二、数据层 (Data Layer)

### 2.1 核心仓库
```
SettingsRepository (1087 行 - 需重构)
├── 管理 200+ 配置项
├── 使用 SharedPreferences 存储
└── 提供 StateFlow 实时通知

DetectionCache
├── 缓存检测结果避免重复扫描
├── 支持 TTL 过期策略
└── ⚠️ 存在线程安全问题 (待修复)

FingerprintDatabase
├── 设备指纹基线存储
└── SQLite 实现

RootDetectRepositoryImpl
└── 实现 IRootDetectRepository 接口
```

### 2.2 JNI 桥接
```
NativeBridge
├── isDeviceRooted(): Boolean
├── detectXposedFramework(): Boolean
├── fullMemoryFingerprint(): Long
├── detectSyscallResultInconsistency(): Boolean
├── detectAVBStatus(): Boolean
├── detectCustomRecovery(): Boolean
├── detectFrida(): Boolean
├── detectVirtualXposed(): Boolean
├── detectGameGuardian(): Boolean
├── detectCheatEngine(): Boolean
├── detectLuckyPatcher(): Boolean
├── detectMemoryEditors(): Boolean
├── detectCrackingTools(): Boolean
├── detectMagiskDenyList(): Boolean
├── detectZygiskModules(): Boolean
├── detectLSPosedManager(): Boolean
├── detectRiruModules(): Boolean
├── detectModernForks(): Boolean
├── detectModernRootForksV2(): Boolean
├── detectAPatchKPM(): Boolean
├── detectAPatchTrampoline(): Boolean
├── detectKernelPatchProject(): Boolean
├── detectZygiskAssistant(): Boolean
├── detectPersistentScripts(): Boolean
├── detectHideMyApplist(): Boolean
├── detectStorageIsolation(): Boolean
└── detectModernHookFrameworks(): Boolean
```

### 2.3 IPC 通信
```
DetectionProtocol
├── MAGIC_REPORT: Byte (0x01)
├── MAGIC_ALERT: Byte (0x02)
├── MAGIC_PROGRESS: Byte (0x03)
├── encodeScanTask(task): ByteArray
├── decodeReport(data): GlobalSecureReport?
├── decodeAlert(data): SecurityAlert?
└── ⚠️ 协议解析缺陷：raw > maxLen 时少读数据导致流错位

SecureSocketClient
├── connect(): Boolean
├── disconnect(): suspend Unit
├── closeNow(): Unit
├── send(data): Boolean
├── messages: StateFlow<ByteArray>
└── connectionState: StateFlow<Boolean>
```

---

## 三、业务逻辑层 (Domain Logic)

### 3.1 并行检测引擎
```
ParallelDetectionEngine
├── TOTAL_LAYERS: Int (19)
├── progress: StateFlow<Int> (0-100)
├── currentLayer: StateFlow<String>
├── isScanning: StateFlow<Boolean>
├── scanParallel(): ParallelScanResult
├── scanSingleLayer(layerId): LayerResult?
├── scanSubset(layerIds): List<LayerResult>
└── reset(): Unit

⚠️ 设计缺陷：L1/L4/L6/L8/L9/L10/L12 都调用同一个 NativeBridge.isDeviceRooted()
   导致分层检测失去意义，权重设计也不合理
```

### 3.2 交叉验证器
```
CrossValidator
└── 多副本共识验证机制
```

### 3.3 设备指纹基线
```
DeviceFingerprintBaseline
└── 建立和比对设备指纹
```

### 3.4 媒体扫描器
```
MediaScanner
└── 扫描媒体文件异常
```

### 3.5 实时防护监控
```
RealtimeGuardMonitor
└── 实时监控防护状态
```

---

## 四、视图模型层 (ViewModel Layer)

### 4.1 扫描视图模型
```
ScanViewModel
├── uiState: StateFlow<UiState>
│   ├── Idle
│   ├── Connecting
│   ├── Scanning
│   ├── Report(GlobalSecureReport)
│   ├── Alert(SecurityAlert)
│   └── Error(String)
├── progress: StateFlow<Float> (0-1)
├── alerts: StateFlow<List<SecurityAlert>> (历史警报)
├── connect(): Unit
├── startScan(level): Unit
├── dismissReport(): Unit
├── dismissAlert(): Unit
├── dismissAlertFromHistory(index): Unit
├── clearAllAlerts(): Unit
└── onCleared(): Unit

✅ 已修复：超时 Job 泄漏问题
✅ 已修复：client.closeNow() 替代 suspend disconnect()
```

### 4.2 设置视图模型
```
SettingsViewModel
└── 管理设置页面状态
```

### 4.3 Apex 视图模型
```
ApexViewModel
└── 主应用状态管理
```

### 4.4 更新视图模型
```
UpdateViewModel
└── 应用更新检查和管理
```

---

## 五、UI 层 (M3 重构进行中)

### 5.1 当前状态 (v1.0.4)
- **旧 UI 已删除**: Glass/Liquid 风格组件已全部移除
- **M3 UI 已搭建**: Material Design 3 主题 + 核心组件 + Dashboard + Settings
- **重构原则**: 高质量优先，速度次要

### 5.2 M3 UI 架构 (v1.0.4)
```
MainActivity
  └─ ApexRootTheme (M3 主题, 支持动态色彩)
       └─ ApexRootApp (导航入口)
            └─ NavHost
                 ├─ "dashboard" → DashboardScreen
                 │    └─ DashboardViewModel
                 │         └─ ScanRepository → NativeBridge (JNI)
                 └─ "settings" → SettingsScreen
                      └─ SettingsViewModel
                           └─ SettingsRepository → SettingsDataSource (DataStore)
```

### 5.3 已完成的 M3 组件
| 组件 | 文件 | 说明 |
|------|------|------|
| Theme | `ui/theme/Theme.kt` | M3 主题 (支持 Android 12+ 动态色彩) |
| Color | `ui/theme/Color.kt` | M3 色板 (Light/Dark) |
| Typography | `ui/theme/Typography.kt` | M3 字体 |
| Shape | `ui/theme/Shape.kt` | M3 形状 |
| ApexTopAppBar | `ui/components/ApexTopAppBar.kt` | 顶部栏组件 |
| ApexCard | `ui/components/ApexCard.kt` | 卡片组件 |
| ApexButton | `ui/components/ApexButton.kt` | 按钮组件 |
| ApexListItem | `ui/components/ApexListItem.kt` | 列表项组件 |

### 5.4 已完成的页面
| 页面 | 文件 | 状态 | ViewModel |
|------|------|------|-----------|
| Dashboard | `ui/screens/dashboard/DashboardScreen.kt` | ✅ 基础完成 | DashboardViewModel |
| Settings | `ui/screens/settings/SettingsScreen.kt` | ✅ 基础完成 | SettingsViewModel |

### 5.5 待完善的页面
| 屏幕名称 | 优先级 | 备注 |
|---------|--------|------|
| ScanResultScreen | P0 | 20 层检测结果展示 (LazyColumn) |
| GuardMonitorScreen | P1 | 实时监控开关 + 告警列表 |
| AboutScreen | P2 | 版本信息 + GitHub 链接 |
| PermissionsScreen | P2 | 权限管理 |
| UpdateScreen | P2 | 更新管理 |

### 5.6 ViewModel 与 Repository 数据流
```
DashboardViewModel
  ├─ ScanRepository
  │    ├─ NativeBridge (JNI → C++ 20层检测)
  │    ├─ getSystemProperty() → getprop 命令
  │    └─ fileExists() → File.exists()
  ├─ DeviceRepository
  │    └─ 设备信息 (Build.MODEL / Build.VERSION 等)
  └─ GuardEventRepository
       └─ Room Database (GuardEventEntity)

SettingsViewModel
  └─ SettingsRepository
       └─ SettingsDataSource
            └─ DataStore Preferences
```

### 5.3 待重建的组件库
| 组件类型 | 描述 | 优先级 |

## 六、服务层 (Service Layer)

### 6.1 防护监控服务
```
GuardMonitorService
├── 后台服务运行防护监控
└── ⚠️ scope.cancel() 没有 join，资源可能未完全释放
```

### 6.2 通知系统
```
Notifier
└── 发送系统通知
```

### 6.3 事件总线
```
EventBus
└── 应用内事件分发
```

---

## 七、核心功能模块

### 7.1 自我保护
```
SelfProtection
├── 反调试
├── 反 Hook
├── 完整性校验
└── ⚠️ 大量 catch (_: Exception) {} 静默错误
```

### 7.2 权限管理
```
PermissionManager
└── 运行时权限请求和管理
```

### 7.3 原生库加载
```
NativeLibraryLoader
└── 安全加载 JNI 库
```

### 7.4 原生功能模块
```
NativeGuard      - 防护功能
NativeCure       - 修复功能
NativeHwid       - 硬件 ID 管理
NativeGameMode   - 游戏模式
NativeIsland     - 隔离岛功能
HideModeManager  - 隐藏模式管理
```

### 7.5 应用更新
```
AppUpdater
└── 检查和下载应用更新
```

### 7.6 报告导出
```
ReportExporter
└── 导出检测报告为 PDF/HTML
```

### 7.7 修复建议
```
FixRecommendations
└── 根据检测结果生成修复建议
```

---

## 八、Native 层 (C++)

### 8.1 目录结构
```
app/src/main/cpp/
├── trusted_root/          # 可信根模块
│   ├── self_protection/   # 自我保护
│   ├── key_management/    # 密钥管理
│   ├── crypto/            # 加密原语 (OQS 签名)
│   └── signing/           # 签名中心
├── detect/                # 检测层 (L1-L20)
│   ├── layer1_prop.*      # L1: 系统属性
│   ├── layer2_art.*       # L2: ART 注入
│   ├── layer3_mem.*       # L3: 内存特征
│   ├── layer4_mount.*     # L4: 挂载检查
│   ├── layer5_sidechannel.* # L5: 侧信道
│   ├── layer6_kernel.*    # L6: Root 守护
│   ├── layer7_boot.*      # L7: Boot 状态
│   ├── layer8_magisk.*    # L8: Magisk
│   ├── layer9_ksu.*       # L9: KernelSU
│   ├── layer10_apatch.*   # L10: APatch
│   ├── layer11_hook.*     # L11: Hook 框架
│   ├── layer12_rom.*      # L12: 自定义 ROM
│   ├── layer13_firmware.* # L13: 固件 (跳过)
│   ├── layer14_virtual.*  # L14: 虚拟框架
│   ├── layer15_dangerous_apps.* # L15: 危险应用
│   ├── layer16_magisk_ext.* # L16: Magisk 扩展
│   ├── layer17_modern_root_forks.* # L17: Root Fork
│   ├── layer18_apatch_kpm.* # L18: APatch KPM
│   ├── layer19_hide.*     # L19: 隐藏框架
│   └── layer20_modern_hooks.* # L20: 现代 Hook
├── bare_syscall/          # 裸 syscall 桥接
├── common/                # 通用工具
└── include/               # 头文件
    ├── plugin_interface.h # 插件接口
    └── apex_common.h      # 通用定义
```

### 8.2 已知 Native 层问题
- `layer1_prop.cpp`: 四个检测函数重复打开/读取/关闭逻辑 (代码重复)
- 部分检测层缺少错误处理

---

## 九、配置项分类 (SettingsRepository)

### 9.1 枚举类型
| 枚举 | 选项数 | 用途 |
|-----|--------|------|
| DetectionLevel | 4 | 检测深度 |
| AutoScanInterval | 4 | 自动扫描间隔 |
| GuardLevel | 3 | 防护等级 |
| HideStrategy | 3 | 隐藏策略 |
| ThemeMode | 3 | 主题模式 |
| AccentColor | 7 | 强调色 |
| UpdateChannel | 3 | 更新渠道 |
| LogLevel | 4 | 日志级别 |
| ScanScheduleTime | 6 | 扫描计划时间 |

### 9.2 配置分类 (200+ 项)
- **检测配置**: 各检测层开关、阈值
- **防护配置**: 防护等级、响应策略
- **UI 配置**: 主题、动画、布局
- **高级配置**: 实验性功能、调试选项
- **隐私配置**: 数据收集、匿名统计

⚠️ **重构建议**: 将 200+ 配置项分组为 20-30 个核心配置，其余移至高级/开发者模式

---

## 十、数据流图

### 10.1 扫描流程
```
用户点击扫描
    ↓
ScanViewModel.startScan(DetectionLevel)
    ↓
编码 ScanTask → SecureSocketClient.send()
    ↓
IPC Socket → Native Daemon
    ↓
ParallelDetectionEngine.scanParallel()
    ↓
并发执行 19 个检测层 (NativeBridge 调用)
    ↓
汇总结果 → GlobalSecureReport
    ↓
编码报告 → IPC Socket → ScanViewModel
    ↓
更新 uiState: UiState.Report(report)
    ↓
UI 展示报告
```

### 10.2 实时警报流程
```
Native Daemon 检测到威胁
    ↓
生成 SecurityAlert
    ↓
IPC Socket → ScanViewModel
    ↓
累积到 alerts 列表 + 更新 uiState: UiState.Alert(alert)
    ↓
UI 展示警报 (当前 + 历史)
```

### 10.3 防护监控流程
```
GuardMonitorService 启动
    ↓
RealtimeGuardMonitor 轮询/监听
    ↓
发现异常 → EventBus 发布事件
    ↓
Notifier 发送通知
    ↓
SelfProtection 触发反制措施
```

---

## 十一、待修复问题清单

### P0 - 严重问题
| ID | 问题 | 位置 | 状态 |
|----|------|------|------|
| P0-1 | DetectionCache 线程安全 | DetectionCache.kt:75-107 | ❌ 待修复 |
| P0-2 | ScanViewModel 资源泄漏 | ScanViewModel.kt:206-216 | ✅ 已修复 |
| P0-3 | ParallelDetectionEngine 检测层设计缺陷 | ParallelDetectionEngine.kt:106-141 | ❌ 待修复 |
| P0-4 | UI 缺失 | 全部 UI 文件 | 🔄 重构中 |

### P1 - 高优先级
| ID | 问题 | 位置 | 状态 |
|----|------|------|------|
| P1-1 | DetectionProtocol 协议解析缺陷 | DetectionProtocol.kt:194-217 | ❌ 待修复 |
| P1-2 | 错误处理过度静默 | 多处 | ❌ 待修复 |
| P1-3 | GuardMonitorService 资源管理不完整 | GuardMonitorService.kt | ❌ 待修复 |

### P2 - 中优先级
| ID | 问题 | 位置 | 状态 |
|----|------|------|------|
| P2-1 | 配置爆炸 (200+ 项) | SettingsRepository.kt | ❌ 待重构 |
| P2-2 | 代码重复 | layer1_prop.cpp | ❌ 待优化 |

---

## 十二、UI 重构路线图

### Phase 0: 基础设施 (当前)
- [x] 删除旧 UI 组件
- [x] 创建知识图谱 (本文档)
- [ ] 创建 UI 重构 SPEC
- [ ] 搭建 M3 主题框架

### Phase 1: 核心组件
- [ ] AppNavigation (导航图)
- [ ] Theme (M3 主题)
- [ ] 基础组件库 (按钮、卡片、输入框等)
- [ ] 通用布局组件

### Phase 2: 核心页面
- [ ] SplashScreen (启动页)
- [ ] DashboardScreen (主仪表板)
- [ ] SettingsScreen (设置页 - 简化版)

### Phase 3: 功能页面
- [ ] ReportScreen
- [ ] AlertScreen
- [ ] GuardMonitorScreen
- [ ] PermissionsScreen
- [ ] WhitelistScreen
- [ ] HistoryScreen

### Phase 4: 辅助页面
- [ ] UpdateScreen
- [ ] AboutScreen
- [ ] ConfigScreen
- [ ] 其他 P3 优先级页面

### Phase 5: 清理和优化
- [ ] 删除遗留代码
- [ ] 性能优化
- [ ] 可访问性改进
- [ ] 国际化支持

---

## 十三、开发规范

### 13.1 代码风格
- Kotlin: 遵循官方 Kotlin 风格指南
- Compose: 遵循 Compose 最佳实践
- C++: 遵循 Google C++ 风格指南

### 13.2 架构原则
- 清晰的层级分离 (UI → ViewModel → Domain → Data)
- 单向数据流
- 状态提升
- 不可变数据优先

### 13.3 测试要求
- 单元测试覆盖率 > 70%
- 关键路径必须有测试
- UI 测试覆盖核心用户流程

### 13.4 文档要求
- 公共 API 必须有 KDoc
- 复杂算法必须有注释
- 每次更新后同步知识图谱

---

## 十四、快速参考

### 14.1 关键类速查
| 类名 | 作用 | 路径 |
|-----|------|------|
| ScanResult | 扫描结果模型 | domain/model/ |
| ParallelDetectionEngine | 并行检测引擎 | domain/parallel/ |
| ScanViewModel | 扫描页面 VM | viewmodel/trusted/ |
| NativeBridge | JNI 桥接 | data/jni/ |
| DetectionProtocol | IPC 协议 | ipc/ |

### 14.2 重要常量
| 常量 | 值 | 用途 |
|-----|-----|------|
| TOTAL_LAYERS | 19 | 检测层总数 |
| MAGIC_REPORT | 0x01 | IPC 报告标识 |
| MAGIC_ALERT | 0x02 | IPC 警报标识 |
| MAGIC_PROGRESS | 0x03 | IPC 进度标识 |

### 14.3 风险评分阈值
| 分数范围 | 风险等级 |
|---------|---------|
| 0 | SAFE |
| 1-30 | WARNING |
| 31-60 | DANGER |
| 61-100 | CRITICAL |

---

**维护说明**: 
- 每次完成一个功能模块后，必须更新对应的章节
- 新增文件需在相应章节添加记录
- 修复问题后更新"待修复问题清单"状态
- 保持本文档与代码同步
